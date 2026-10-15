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

namespace {

// Private audio buses from the top of ola's range down: 959, 958, ...
const int kFirstPrivateBus = 512, kEndPrivateBus = 960;
int gBusFloor = kEndPrivateBus;
// Control buses from the top of scsynth's default 16384 down, clear of the low ones sclang takes.
const int kEndControlBus = 16384, kControlBuses = 4096;
int gControlFloor = kEndControlBus;
// Buffers for loaded and allocated data, between the sound files' 0..63 and the wave tables' 512..
const int kFirstDataBuf = 64, kEndDataBuf = 512;
int gNextBuf = kFirstDataBuf;

class Bus : public ServerResource
{
public:
	int mChannels;
	bool mControl;

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

	Buffer(int index, int channels, int64_t frames) : ServerResource(index), mChannels(channels), mFrames(frames) {}
	virtual const char* TypeName() const override { return "Buffer"; }
	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += "Buffer " + std::to_string(mIndex) + " (" + std::to_string(mChannels) + " x " + std::to_string(mFrames) +
			")";
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
	if (gNextBuf == kEndDataBuf) {
		post("%s : all %d data buffers are in use\n", word, kEndDataBuf - kFirstDataBuf);
		throw errOutOfRange;
	}
	return gNextBuf++;
}

void audio_(Thread& th, Prim* prim)
{
	int64_t n = popCount(th, "bus.audio : channels", 64);
	if (gBusFloor - n < kFirstPrivateBus + 64) {
		post("bus.audio : the private buses are in use\n");
		throw errOutOfRange;
	}
	gBusFloor -= (int)n;
	th.push(V(new Bus(gBusFloor, (int)n)));
}

void control_(Thread& th, Prim* prim)
{
	int64_t n = popCount(th, "bus.control : channels", 64);
	if (gControlFloor - n < kEndControlBus - kControlBuses) {
		post("bus.control : all %d control buses are in use\n", kControlBuses);
		throw errOutOfRange;
	}
	gControlFloor -= (int)n;
	th.push(V(new Bus(gControlFloor, (int)n, true)));
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
	th.push(V(new Buffer(s.buf, s.channels, s.frames)));
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

int audioBusFloor() { return gBusFloor; }

void AddResourceOps();
void AddResourceOps()
{
	vm.addBifHelp(std::string("\n*** bus ***"));
	P<Namespace> bus = new Namespace("bus");
	bus->addPrim("audio", 1, audio_, "(channels --> bus) consecutive private audio buses, from 959 down.");
	bus->addPrim("control", 1, control_,
		"(channels --> bus) consecutive control buses, from 16383 down. v bus.set sets them.");
	vm.def("bus", V(bus));
	vm.addBifHelp("bus", nullptr, "(--> namespace) server buses.");
	vm.def("send", 1, 0, send_,
		"(message -->) sends [address args...] to scsynth, as SC's sendMsg: whole reals as ints, others as floats, "
		"strings as strings, a Bus, Buffer or Node as its index.");

	// buffer's UGen words are generated (SpecUGens.cpp); these join them.
	Namespace* ns = generatedNamespace("buffer");
	if (!ns) {
		post("AddResourceOps : no buffer namespace\n");
		throw errFailed;
	}
	vm.addBifHelp(std::string("\n*** buffer: interpreter words ***"));
	ns->addPrim("read", 1, read_, "(path --> buffer) reads a WAV or AIFF file into one of the sound-file buffers 0..63.");
	ns->addPrim("load", 1, load_, "(list --> buffer) a one-channel buffer holding the list's values.");
	ns->addPrim("alloc", 2, alloc_, "(frames channels --> buffer) an empty buffer.");
}
