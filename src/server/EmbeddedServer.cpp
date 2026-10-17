//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "EmbeddedServer.hpp"
#include "Environment.hpp"
#include "Patterns.hpp"
#include "Scsynth.hpp"
#include "Texture.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <unistd.h>

#include "SC_WorldOptions.h"
#include <sndfile.h>
#include "SC_ReplyImpl.hpp"

// HostAudioDriver.cpp, in libscsynth
extern "C" void World_SetHostDriven(bool on);
extern "C" void World_HostRunBlock(World* world, float* const* out, int numOut, int64_t oscTime);

namespace {

const int kBlockFrames = 64;   // scsynth's block size, which the host driver renders at a time
std::string gPluginDir;        // a host's, in place of plugins/ beside the executable

std::string pluginDir()
{
	const char* env = stacEnv("PLUGINS");
	if (env && *env) return env;
	if (!gPluginDir.empty()) return gPluginDir;
	std::error_code ec;
#if defined(__APPLE__)
	char buf[4096];
	uint32_t size = sizeof(buf);
	std::filesystem::path exe = _NSGetExecutablePath(buf, &size) == 0 ? std::filesystem::path(buf) : "";
#else
	std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", ec);
#endif
	return (std::filesystem::canonical(exe, ec).parent_path() / "plugins").string();
}

int envInt(const char* name, int fallback)
{
	const char* env = stacEnv(name);
	return env && *env ? atoi(env) : fallback;
}

// An OSC string at `pos`, advancing past its padding.
std::string oscString(const std::string& in, size_t& pos)
{
	size_t end = in.find('\0', pos);
	if (end == std::string::npos) end = in.size();
	std::string s = in.substr(pos, end - pos);
	pos = (end + 4) & ~(size_t)3;
	return s;
}

// The first `frames` of `from` into `to`, padded with silence: scsynth renders whole blocks, and
// the score's last voices may end early.
std::string copyFrames(const std::string& from, const std::string& to, int channels, int64_t frames, double sr)
{
	SF_INFO in = {};
	SNDFILE* src = sf_open(from.c_str(), SFM_READ, &in);
	if (!src) return "scsynth wrote no file";
	SF_INFO out = {};
	out.samplerate = (int)std::lround(sr);
	out.channels = channels;
	out.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;
	SNDFILE* dst = sf_open(to.c_str(), SFM_WRITE, &out);
	if (!dst) {
		sf_close(src);
		return "cannot write " + to;
	}
	std::vector<float> buf(4096 * (size_t)channels);
	for (int64_t done = 0; done < frames;) {
		sf_count_t n = (sf_count_t)std::min<int64_t>(4096, frames - done);
		sf_count_t got = sf_readf_float(src, buf.data(), n);
		std::fill(buf.begin() + got * channels, buf.begin() + n * channels, 0.f);
		sf_writef_float(dst, buf.data(), n);
		done += n;
	}
	sf_close(src);
	sf_close(dst);
	return "";
}

class EmbeddedServer : public ScsynthServer
{
public:
	std::string sendSync(const std::vector<std::string>& packets, int timeoutMs) override
	{
		std::string error;
		std::unique_lock<std::mutex> lock(mutex_);
		replies_.clear();
		lock.unlock();
		for (const std::string& p : packets)
			if (!sendPacket(p, error)) return error;
		const int syncID = 7301;
		if (!sendPacket(oscMessage("/sync", {{OscArg::Int, syncID, ""}}), error)) return error;
		lock.lock();
		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
		for (;;) {
			while (!replies_.empty()) {
				std::string reply = replies_.front();
				replies_.pop_front();
				size_t pos = 0;
				std::string address = oscString(reply, pos);
				if (address == "/synced") return "";
				if (address == "/fail") {
					oscString(reply, pos); // type tags
					std::string command = oscString(reply, pos);
					return command + ": " + oscString(reply, pos);
				}
			}
			if (cv_.wait_until(lock, deadline) == std::cv_status::timeout && replies_.empty()) return "no reply";
		}
	}

	void send(const std::string& packet) override
	{
		std::string error;
		sendPacket(packet, error);
	}

	std::string address() override { return "this process"; }

	// Host-driven: counts the frames the host has rendered since startHost.
	double now() override
	{
		double rate = hostRate_;
		if (rate <= 0.) return ScsynthServer::now();
		return hostStart_ + (double)hostFrames_ / rate;
	}

	// Refuses sends until startHost.
	void useHost()
	{
		std::lock_guard<std::mutex> lock(worldMutex_);
		hostDriven_ = true;
		World_SetHostDriven(true);
	}

	// Called while the host does not render. The World starts on the first send, as in device mode.
	void startHost(double sampleRate, int outputs)
	{
		stopHost();
		std::lock_guard<std::mutex> lock(worldMutex_);
		hostOutputs_ = outputs;
		block_.assign((size_t)outputs * kBlockFrames, 0.f);
		blockPtrs_.clear();
		for (int k = 0; k < outputs; ++k) blockPtrs_.push_back(block_.data() + k * kBlockFrames);
		blockPos_ = kBlockFrames;
		hostStart_ = ScsynthServer::now();
		hostFrames_ = 0;
		hostRate_ = sampleRate;
		closed_ = false;
	}

	// Frees the synths and stops the World, as shutdown does, and refuses sends until startHost.
	void stopHost()
	{
		shutdown();
		while (rendering_) std::this_thread::yield();
	}

	// On the host's audio thread. Silence until the World starts.
	void renderHost(float* const* out, int channels, int frames)
	{
		rendering_ = true;
		World* world = hostWorld_;
		for (int i = 0; i < frames;) {
			if (blockPos_ == kBlockFrames) {
				if (world) World_HostRunBlock(world, blockPtrs_.data(), hostOutputs_, oscTime(now()));
				else std::fill(block_.begin(), block_.end(), 0.f);
				hostFrames_ += kBlockFrames;
				blockPos_ = 0;
			}
			int n = std::min(kBlockFrames - blockPos_, frames - i);
			for (int k = 0; k < channels; ++k) {
				if (k < hostOutputs_) std::copy_n(blockPtrs_[k] + blockPos_, n, out[k] + i);
				else std::fill_n(out[k] + i, n, 0.f);
			}
			i += n;
			blockPos_ += n;
		}
		rendering_ = false;
	}

	std::string render(const std::string& score, const std::string& path, int channels, int64_t frames,
		double sr) override
	{
		namespace fs = std::filesystem;
		static int count = 0;
		std::error_code ec;
		fs::path tmp = fs::temp_directory_path(ec) / ("sapf-render-" + std::to_string(getpid()) + "-" +
			std::to_string(++count));
		std::string cmd = tmp.string() + ".osc", raw = tmp.string() + ".wav";
		{
			std::ofstream f(cmd, std::ios::binary);
			if (!f.write(score.data(), (std::streamsize)score.size())) return "cannot write " + cmd;
		}
		std::string plugins = pluginDir();
		WorldOptions o;
		o.mRealTime = false;
		o.mPreferredSampleRate = (uint32_t)std::lround(sr);
		o.mNumOutputBusChannels = (uint32_t)channels;
		o.mNumInputBusChannels = 0;
		o.mUGensPluginPath = plugins.c_str();
		o.mLoadGraphDefs = 0;
		o.mRendezvous = false;
		o.mVerbosity = -1;
		o.mNonRealTimeCmdFilename = cmd.c_str();
		o.mNonRealTimeOutputFilename = raw.c_str();
		o.mNonRealTimeOutputHeaderFormat = "WAV";
		o.mNonRealTimeOutputSampleFormat = "float";
		World* world = World_New(&o);
		std::string error;
		if (!world) error = "scsynth did not start for rendering";
		else World_NonRealTimeSynthesis(world, &o);  // cleans up the World
		fs::remove(cmd, ec);
		if (error.empty()) error = copyFrames(raw, path, channels, frames, sr);
		fs::remove(raw, ec);
		return error;
	}

	// The synths running, from /status.reply's third argument; -1 if the server is not running.
	int synths()
	{
		{
			std::lock_guard<std::mutex> lock(worldMutex_);
			if (!world_) return -1;
		}
		std::unique_lock<std::mutex> lock(mutex_);
		replies_.clear();
		lock.unlock();
		std::string error;
		if (!sendPacket(oscMessage("/status", {}), error)) return -1;
		lock.lock();
		auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
		for (;;) {
			while (!replies_.empty()) {
				std::string reply = replies_.front();
				replies_.pop_front();
				size_t pos = 0;
				if (oscString(reply, pos) != "/status.reply") continue;
				oscString(reply, pos);  // type tags; then ints: unused, UGens, synths
				pos += 8;
				if (pos + 4 > reply.size()) return -1;
				const auto* b = (const unsigned char*)reply.data() + pos;
				return (int)((uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3]);
			}
			if (cv_.wait_until(lock, deadline) == std::cv_status::timeout && replies_.empty()) return -1;
		}
	}

private:
	// Starts the World on first use. The caller holds worldMutex_.
	bool boot(std::string& error)
	{
		if (world_) return true;
		if (closed_ || (hostDriven_ && hostRate_ <= 0.)) {
			error = hostDriven_ ? "the host is not running audio" : "scsynth has shut down";
			return false;
		}
		std::string plugins = pluginDir(), device;
		if (const char* d = stacEnv("DEVICE")) device = d;
		WorldOptions o;
		o.mRealTime = true;
		o.mNumOutputBusChannels = envInt("OUTPUTS", 2);
		o.mNumInputBusChannels = envInt("INPUTS", 0);  // an input stream can block boot when another client holds the device
		o.mPreferredSampleRate = envInt("SR", 0);
		o.mBufLength = kBlockFrames;
		if (hostDriven_) {
			o.mNumOutputBusChannels = hostOutputs_;
			o.mNumInputBusChannels = 0;
			o.mPreferredSampleRate = (uint32_t)std::lround(hostRate_);
			device.clear();
		}
		o.mUGensPluginPath = plugins.c_str();
		o.mLoadGraphDefs = 0;  // else it loads every def in SC's default folders
		o.mRendezvous = false;
		o.mMemoryLocking = false;
		if (!device.empty()) {
			o.mInDeviceName = device.c_str();
			o.mOutDeviceName = device.c_str();
		}
		world_ = World_New(&o);
		if (!world_) {
			error = "scsynth did not start; check the audio device, or set STAC_SCSYNTH to use a running server";
			return false;
		}
		if (hostDriven_) {
			hostWorld_ = world_;
			return true;  // the host stops it: onExit's _exit would end the host's process
		}
		gSelf = this;
		std::atexit(onExit);
		return true;
	}

	// The lock keeps shutdown from freeing the World while a player thread sends to it.
	bool sendPacket(const std::string& packet, std::string& error)
	{
		std::lock_guard<std::mutex> lock(worldMutex_);
		if (!boot(error)) return false;
		std::string copy = packet;  // World_SendPacket takes a mutable buffer and copies it
		World_SendPacketWithContext(world_, (int)copy.size(), copy.data(), onReply, this);
		return true;
	}

	// Refuses later sends and boots, then stops the audio, frees the synths and unloads the
	// plugins, running their unload hooks; patch-supercollider.sh fixes World_Cleanup's order.
	// Host-driven, the plugins stay loaded: the host may start another World, whose units would
	// call the unloaded code.
	void shutdown()
	{
		World* world;
		{
			std::lock_guard<std::mutex> lock(worldMutex_);
			closed_ = true;
			world = std::exchange(world_, nullptr);
			hostWorld_ = nullptr;
		}
		if (world) World_Cleanup(world, !hostDriven_);
	}

	static void onReply(ReplyAddress* addr, char* buf, int size)
	{
		auto* self = static_cast<EmbeddedServer*>(addr->mReplyData);
		std::lock_guard<std::mutex> lock(self->mutex_);
		self->replies_.emplace_back(buf, (size_t)size);
		self->cv_.notify_all();
	}

	// Players stop first, so none reports a failed send. CoreAudio's static destructors can crash
	// after the World stops, so on macOS the process ends here, as nanosynth does.
	static void onExit()
	{
		stopTextures();
		stopEventStreams();
		gSelf->shutdown();
		fflush(nullptr);
#if defined(__APPLE__)
		_exit(0);
#endif
	}

	// OSC time, NTP 32.32 fixed point, of `seconds` since 1970, as oscBundle writes it.
	static int64_t oscTime(double seconds)
	{
		double t = seconds + 2208988800.;
		double whole = std::floor(t);
		return (int64_t)(((uint64_t)whole << 32) + (uint64_t)std::llround((t - whole) * 4294967296.));
	}

	static inline EmbeddedServer* gSelf = nullptr;
	World* world_ = nullptr;
	bool closed_ = false;
	std::mutex worldMutex_, mutex_;
	std::condition_variable cv_;
	std::deque<std::string> replies_;

	bool hostDriven_ = false;
	std::atomic<double> hostRate_{0.}, hostStart_{0.};
	int hostOutputs_ = 0;
	std::atomic<int64_t> hostFrames_{0};
	std::atomic<World*> hostWorld_{nullptr};
	std::atomic<bool> rendering_{false};
	// the audio thread's block, of which it has played blockPos_ frames
	std::vector<float> block_;
	std::vector<float*> blockPtrs_;
	int blockPos_ = kBlockFrames;
};

} // namespace

// Never destroyed: detached player threads may still send while the process exits.
static EmbeddedServer& gServer = *new EmbeddedServer;

void registerEmbeddedScsynth()
{
	setEmbeddedScsynth(&gServer);
}

void registerHostDrivenScsynth(const std::string& pluginDir)
{
	gPluginDir = pluginDir;
	gServer.useHost();
	setEmbeddedScsynth(&gServer);
}

void startHostDrivenScsynth(double sampleRate, int outputs)
{
	gServer.startHost(sampleRate, outputs);
}

void stopHostDrivenScsynth()
{
	gServer.stopHost();
}

void renderHostDrivenScsynth(float* const* out, int channels, int frames)
{
	gServer.renderHost(out, channels, frames);
}

void waitForEmbeddedScsynth()
{
	while (gServer.synths() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
