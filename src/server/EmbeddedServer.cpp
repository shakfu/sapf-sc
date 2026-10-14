//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "EmbeddedServer.hpp"
#include "Scsynth.hpp"

#include <chrono>
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
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#include <unistd.h>

#include "SC_WorldOptions.h"
#include <sndfile.h>
#include "SC_ReplyImpl.hpp"

namespace {

std::string pluginDir()
{
	const char* env = getenv("SAPF_PLUGINS");
	if (env && *env) return env;
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
	const char* env = getenv(name);
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
		if (!boot(error)) return error;
		std::unique_lock<std::mutex> lock(mutex_);
		replies_.clear();
		lock.unlock();
		for (const std::string& p : packets) sendPacket(p);
		const int syncID = 7301;
		sendPacket(oscMessage("/sync", {{OscArg::Int, syncID, ""}}));
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
		if (boot(error)) sendPacket(packet);
	}

	std::string address() override { return "this process"; }

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
		if (!world_) return -1;
		std::unique_lock<std::mutex> lock(mutex_);
		replies_.clear();
		lock.unlock();
		sendPacket(oscMessage("/status", {}));
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
	bool boot(std::string& error)
	{
		std::lock_guard<std::mutex> lock(bootMutex_);
		if (world_) return true;
		std::string plugins = pluginDir(), device;
		if (const char* d = getenv("SAPF_DEVICE")) device = d;
		WorldOptions o;
		o.mRealTime = true;
		o.mNumOutputBusChannels = envInt("SAPF_OUTPUTS", 2);
		o.mNumInputBusChannels = envInt("SAPF_INPUTS", 2);
		o.mPreferredSampleRate = envInt("SAPF_SR", 0);
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
			error = "scsynth did not start; check the audio device, or set SAPF_SCSYNTH to use a running server";
			return false;
		}
		gWorld = world_;
		std::atexit(quit);
		return true;
	}

	void sendPacket(const std::string& packet)
	{
		std::string copy = packet;  // World_SendPacket takes a mutable buffer and copies it
		World_SendPacketWithContext(world_, (int)copy.size(), copy.data(), onReply, this);
	}

	static void onReply(ReplyAddress* addr, char* buf, int size)
	{
		auto* self = static_cast<EmbeddedServer*>(addr->mReplyData);
		std::lock_guard<std::mutex> lock(self->mutex_);
		self->replies_.emplace_back(buf, (size_t)size);
		self->cv_.notify_all();
	}

	// CoreAudio's static destructors can crash after the World stops, so on macOS the process
	// ends here, after flushing what it printed, as nanosynth does.
	static void quit()
	{
		fflush(nullptr);
#if defined(__APPLE__)
		_exit(0);
#else
		World_Cleanup(gWorld, true);
#endif
	}

	static inline World* gWorld = nullptr;
	World* world_ = nullptr;
	std::mutex bootMutex_, mutex_;
	std::condition_variable cv_;
	std::deque<std::string> replies_;
};

} // namespace

static EmbeddedServer gServer;

void registerEmbeddedScsynth()
{
	setEmbeddedScsynth(&gServer);
}

void waitForEmbeddedScsynth()
{
	while (gServer.synths() > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
