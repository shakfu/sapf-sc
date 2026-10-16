//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// The bus namespace, buffer's interpreter words and send (docs/dev/stac.md, Server objects).

#include "Def.hpp"
#include "Namespace.hpp"
#include "Resources.hpp"
#include "Scsynth.hpp"
#include <algorithm>
#include <cmath>
#include <map>

namespace {

// Runs of consecutive indices in [lo, hi), taken and freed as sclang's ContiguousBlockAllocator
// does. A top-down allocator takes the highest free run that fits; a bottom-up one the lowest.
class Blocks
{
public:
	Blocks(int lo, int hi, bool down) : mLo(lo), mHi(hi), mDown(down) {}

	// The first index of n free ones, or -1.
	int take(int n)
	{
		int at = -1;
		if (mDown) {
			int end = mHi;
			for (auto it = mUsed.rbegin(); it != mUsed.rend() && at < 0; end = it->first, ++it)
				if (end - (it->first + it->second) >= n) at = end - n;
			if (at < 0 && end - mLo >= n) at = end - n;
		} else {
			int start = mLo;
			for (auto it = mUsed.begin(); it != mUsed.end() && at < 0; start = it->first + it->second, ++it)
				if (it->first - start >= n) at = start;
			if (at < 0 && mHi - start >= n) at = start;
		}
		if (at >= 0) mUsed[at] = n;
		return at;
	}

	void give(int at) { mUsed.erase(at); }

	// The lowest index taken, or hi.
	int floor() const { return mUsed.empty() ? mHi : mUsed.begin()->first; }

private:
	int mLo, mHi;
	bool mDown;
	std::map<int, int> mUsed;  // first index: count
};

// Private audio buses from 959 down; ola rotates through 512 and up, and keeps 64 below them.
const int kFirstPrivateBus = 512, kEndPrivateBus = 960;
Blocks gAudioBuses(kFirstPrivateBus + 64, kEndPrivateBus, true);
// Control buses from the top of scsynth's default 16384 down, clear of the low ones sclang takes.
const int kEndControlBus = 16384, kControlBuses = 4096;
Blocks gControlBuses(kEndControlBus - kControlBuses, kEndControlBus, true);
// Buffers for loaded and allocated data, between the sound files' 0..63 and the wave tables' 512..
const int kFirstDataBuf = 64, kEndDataBuf = 512;
Blocks gDataBufs(kFirstDataBuf, kEndDataBuf, false);

class Bus : public ServerResource
{
public:
	int mChannels;
	bool mControl;
	bool mFreed = false;

	Bus(int index, int channels, bool control = false) : ServerResource(index), mChannels(channels), mControl(control) {}
	virtual const char* TypeName() const override { return "Bus"; }
	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += "Bus " + std::to_string(mIndex) + " (" + std::to_string(mChannels) + (mControl ? " control)" : " audio)");
	}

	// v b.set: SC's Bus.set, a real or a list of one per channel, sent with /c_setn.
	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (name && !strcmp(name, "free")) {
			// SC's Bus.free: the buses return to the allocator; synths still reading them are not told.
			if (mFreed) {
				post("free : bus %d is already free\n", mIndex);
				throw errFailed;
			}
			(mControl ? gControlBuses : gAudioBuses).give(mIndex);
			mFreed = true;
			ioValue = noValue();
			return true;
		}
		if (!name || strcmp(name, "set")) return false;
		if (!mControl) {
			post("set : bus %d is an audio bus; only a control bus holds a value\n", mIndex);
			throw errFailed;
		}
		V v = th.pop();
		std::vector<V> values;
		if (v.isVList() && v.isFinite()) {
			P<List> list = ((List*)v.o())->pack(th);
			for (int64_t i = 0; i < list->mArray->size(); ++i) values.push_back(list->mArray->at(i));
		} else {
			values.push_back(v);
		}
		if ((int)values.size() > mChannels) {
			post("set : %d values for a bus of %d channels\n", (int)values.size(), mChannels);
			throw errOutOfRange;
		}
		std::vector<OscArg> args = {{OscArg::Int, mIndex, ""}, {OscArg::Int, (int)values.size(), ""}};
		for (Arg x : values) {
			if (!x.isReal()) wrongType("set : value", "Real", x);
			args.push_back({OscArg::Float, 0, "", (float)x.f});
		}
		checkServer(scsynthSendSync({oscMessage("/c_setn", args)}), "set");
		ioValue = noValue();
		return true;
	}
};

class Buffer : public ServerResource
{
public:
	int mChannels;
	int64_t mFrames;
	bool mFile;  // a sound file's buffer, 0..63, from buffer.read
	bool mFreed = false;

	Buffer(int index, int channels, int64_t frames, bool file = false)
		: ServerResource(index), mChannels(channels), mFrames(frames), mFile(file) {}
	virtual const char* TypeName() const override { return "Buffer"; }
	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += "Buffer " + std::to_string(mIndex) + " (" + std::to_string(mChannels) + " x " + std::to_string(mFrames) +
			")";
	}

	// SC's Buffer.free: /b_free, and the index returns to its allocator.
	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (!name || strcmp(name, "free")) return false;
		if (mFreed) {
			post("free : buffer %d is already free\n", mIndex);
			throw errFailed;
		}
		checkServer(scsynthSendSync({oscMessage("/b_free", {{OscArg::Int, mIndex, ""}})}), "free");
		if (mFile) releaseSampleBuffer(mIndex);
		else gDataBufs.give(mIndex);
		mFreed = true;
		ioValue = noValue();
		return true;
	}
};


int64_t popCount(Thread& th, const char* msg, int64_t max)
{
	int64_t n = th.popInt(msg);
	if (n < 1 || n > max) {
		post("%s : from 1 to %lld\n", msg, (long long)max);
		throw errOutOfRange;
	}
	return n;
}

int takeBuffer(const char* word)
{
	int buf = gDataBufs.take(1);
	if (buf < 0) {
		post("%s : all %d data buffers are in use\n", word, kEndDataBuf - kFirstDataBuf);
		throw errOutOfRange;
	}
	return buf;
}

void audio_(Thread& th, Prim* prim)
{
	int64_t n = popCount(th, "bus.audio : channels", 64);
	th.push(V(new Bus(takeAudioBuses((int)n, "bus.audio"), (int)n)));
}

void control_(Thread& th, Prim* prim)
{
	int64_t n = popCount(th, "bus.control : channels", 64);
	int bus = gControlBuses.take((int)n);
	if (bus < 0) {
		post("bus.control : no %d free control buses of %d\n", (int)n, kControlBuses);
		throw errOutOfRange;
	}
	th.push(V(new Bus(bus, (int)n, true)));
}

// [address args...] send: SC's sendMsg. Whole reals go as ints, others as floats, as sclang's
// Integer and Float; a Bus, Buffer or Node as its index.
void send_(Thread& th, Prim* prim)
{
	V v = th.pop();
	if (!v.isVList() || !v.isFinite()) wrongType("send : message", "finite list", v);
	P<List> list = ((List*)v.o())->pack(th);
	Array* a = list->mArray();
	const char* address = a->size() ? symbolOf(a->at(0)) : nullptr;
	if (!address || address[0] != '/') wrongType("send : address", "String starting with /", a->size() ? a->at(0) : v);
	std::vector<OscArg> args;
	for (int64_t i = 1; i < a->size(); ++i) {
		V x = a->at(i);
		if (const char* s = symbolOf(x)) {
			args.push_back({OscArg::String, 0, s});
		} else if (ServerResource* r = asResource(x)) {
			args.push_back({OscArg::Int, r->mIndex, ""});
		} else if (x.isReal()) {
			double f = x.f;
			if (f == std::floor(f) && std::fabs(f) < 2147483648.) args.push_back({OscArg::Int, (int)f, ""});
			else args.push_back({OscArg::Float, 0, "", (float)f});
		} else {
			wrongType("send : argument", "Real, String, Bus, Buffer or Node", x);
		}
	}
	checkServer(scsynthSendSync({oscMessage(address, args)}), "send");
}

void read_(Thread& th, Prim* prim)
{
	SampleInfo s = sampleBuffer(th, th.pop(), "buffer.read");
	checkServer(scsynthSendSync({oscMessage("/b_allocRead", {{OscArg::Int, s.buf, ""}, {OscArg::String, 0, s.absolute}})}),
		"buffer.read");
	th.push(V(new Buffer(s.buf, s.channels, s.frames, true)));
}

void load_(Thread& th, Prim* prim)
{
	V v = th.pop();
	if (!v.isList() || !((List*)v.o())->isFinite()) wrongType("buffer.load : values", "finite list of reals", v);
	P<List> list = ((List*)v.o())->pack(th);
	int64_t n = list->mArray->size();
	if (n == 0) {
		post("buffer.load : no values\n");
		throw errOutOfRange;
	}
	int buf = takeBuffer("buffer.load");
	// /b_alloc completes asynchronously; /b_setn sent before it ends would write to no buffer.
	checkServer(scsynthSendSync({oscMessage("/b_alloc", {{OscArg::Int, buf, ""}, {OscArg::Int, (int)n, ""},
		{OscArg::Int, 1, ""}})}), "buffer.load");
	std::vector<std::string> packets;
	// /b_setn in chunks that fit a UDP packet
	for (int64_t i = 0; i < n; i += 1024) {
		std::vector<OscArg> args = {{OscArg::Int, buf, ""}, {OscArg::Int, (int)i, ""},
			{OscArg::Int, (int)std::min<int64_t>(1024, n - i), ""}};
		for (int64_t j = i; j < std::min<int64_t>(i + 1024, n); ++j) {
			V x = list->mArray->at(j);
			if (!x.isReal()) wrongType("buffer.load : values", "Real", x);
			args.push_back({OscArg::Float, 0, "", (float)x.f});
		}
		packets.push_back(oscMessage("/b_setn", args));
	}
	checkServer(scsynthSendSync(packets), "buffer.load");
	th.push(V(new Buffer(buf, 1, n)));
}

void alloc_(Thread& th, Prim* prim)
{
	int64_t channels = popCount(th, "buffer.alloc : channels", 64);
	int64_t frames = popCount(th, "buffer.alloc : frames", (int64_t)1 << 28);
	int buf = takeBuffer("buffer.alloc");
	checkServer(scsynthSendSync({oscMessage("/b_alloc", {{OscArg::Int, buf, ""}, {OscArg::Int, (int)frames, ""},
		{OscArg::Int, (int)channels, ""}})}), "buffer.alloc");
	th.push(V(new Buffer(buf, (int)channels, frames)));
}

} // namespace

int audioBusFloor() { return gAudioBuses.floor(); }

int takeAudioBuses(int n, const char* word)
{
	int bus = gAudioBuses.take(n);
	if (bus < 0) {
		post("%s : no %d free private buses\n", word, n);
		throw errOutOfRange;
	}
	return bus;
}

void AddResourceOps();
void AddResourceOps()
{
	vm.addBifHelp(std::string("\n*** bus ***"));
	P<Namespace> bus = new Namespace("bus");
	bus->addPrim("audio", 1, audio_, "(channels --> bus) consecutive private audio buses, from 959 down. bus.free returns them.");
	bus->addPrim("control", 1, control_,
		"(channels --> bus) consecutive control buses, from 16383 down. v bus.set sets them; bus.free returns them.");
	vm.def("bus", V(bus));
	vm.addBifHelp("bus", nullptr, "(--> namespace) server buses.");
	vm.def("send", 1, 0, send_,
		"(message -->) sends [address args...] to scsynth, as SC's sendMsg: whole reals as ints, others as floats, "
		"strings as strings, a Bus, Buffer or Node as its index.");

	// SC's Buffer class methods; the buffer UGens are in sc.
	vm.addBifHelp(std::string("\n*** buffer ***"));
	P<Namespace> ns = new Namespace("buffer");
	ns->addPrim("read", 1, read_, "(path --> buffer) reads a WAV or AIFF file into one of the sound-file buffers 0..63. buffer.free frees it.");
	ns->addPrim("load", 1, load_, "(list --> buffer) a one-channel buffer holding the list's values.");
	ns->addPrim("alloc", 2, alloc_, "(frames channels --> buffer) an empty buffer.");
	vm.def("buffer", V(ns));
	vm.addBifHelp("buffer", nullptr, "(--> namespace) server buffers.");
}
