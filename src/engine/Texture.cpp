//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Texture.hpp"
#include "Namespace.hpp"
#include "Resources.hpp"
#include "Scsynth.hpp"
#include "Samples.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <map>
#include <thread>

extern P<String> s_out;
extern P<String> s_dt;

namespace {

// Private audio buses for ola, above scsynth's hardware buses and below the 32 that the SSP's scsy
// keeps for controls; scsynth has 1024 by default.
const int kFirstBus = 512, kLastBus = 960;
int gNextBus = kFirstBus;
std::atomic<int64_t> gVoiceCount{0};
std::atomic<int> gGeneration{0};
std::atomic<int> gRunning{0};

// The voice stream of one ola. Copies share the sounds and hops, each with its own position.
class Texture : public Object
{
public:
	V sounds, hops;
	double rate;
	int numChannels, bus;

	Texture(Arg inSounds, Arg inHops, double inRate, int inNumChannels, int inBus)
		: sounds(inSounds), hops(inHops), rate(inRate), numChannels(inNumChannels), bus(inBus) {}

	virtual const char* TypeName() const override { return "Texture"; }
};

// Walks a texture's voices as sapf's OverlapAdd does: the sound for event i (a function is
// called with i), then the hop to the next event, in beats divided by rate.
struct Voices {
	P<Texture> tx;
	VIn sounds;
	BothIn hops;
	double beat = 0.;
	int64_t counter = 0;

	explicit Voices(P<Texture> const& inTx) : tx(inTx)
	{
		sounds.set(tx->sounds);
		hops.set(tx->hops);
	}

	// The next voice's channels and its time in seconds. A sound that is not a signal is skipped, as
	// in sapf.
	bool next(Thread& th, double& seconds, V& out)
	{
		for (;;) {
			V sound;
			if (sounds.one(th, sound)) return false;
			if (sound.isFun()) {
				SaveStack ss(th);
				th.push((double)counter);
				sound.apply(th);
				sound = th.pop();
			}
			++counter;
			Z hop;
			if (hops.onez(th, hop)) return false;
			out = sound;
			if (sound.isForm()) {
				sound.dot(th, s_out, out);
				V dt;
				if (sound.dot(th, s_dt, dt) && dt.isReal()) hop = dt.f;
			}
			seconds = beat / tx->rate;
			beat += hop;
			if (!isSig(out) && !(out.isVList() && out.isFinite())) continue;
			if (out.isVList()) {
				int64_t n = tx->numChannels;
				out = ((List*)out.o())->packSome(th, n);
			}
			if (!texturesOf(th, out).empty()) {
				post("ola : a voice cannot contain an ola\n");
				throw errFailed;
			}
			return true;
		}
	}

	std::string compile(Thread& th, Arg out, const std::string& name) { return compileSynthDef(th, out, name, tx->bus); }
};

std::string voiceName() { return "sapf-voice-" + std::to_string(++gVoiceCount); }

std::string newSynth(const std::string& name, int addAction)
{
	return oscMessage("/s_new", {{OscArg::String, 0, name}, {OscArg::Int, -1, ""}, {OscArg::Int, addAction, ""},
		{OscArg::Int, 0, ""}});
}

std::string defRecv(const std::string& def) { return oscMessage("/d_recv", {{OscArg::Blob, 0, def}}); }
std::string defFree(const std::string& name) { return oscMessage("/d_free", {{OscArg::String, 0, name}}); }

// Each voice's def goes ahead of its start, and the bundle that starts it frees the def; the
// synth keeps it. scsynth starts a synth at the start of the block holding its bundle's time.
// OffsetOut would start it on the exact frame, but crashes scsynth's NRT renderer.
const double kLookahead = .2;

void schedule(std::shared_ptr<Thread> th, P<Texture> tx, double startTime, int generation,
	std::shared_ptr<LoadedTables> loaded, int mainNode)
{
	struct Running {  // playTextures counted this thread before starting it
		~Running() { --gRunning; }
	} running;
	Voices voices(tx);
	try {
		double seconds;
		V out;
		while (generation == gGeneration && voices.next(*th, seconds, out)) {
			std::string name = voiceName(), def = voices.compile(*th, out, name);
			// on the server's clock, which may not keep the wall clock's pace; stop ends the wait
			const double wake = startTime + seconds - kLookahead;
			for (double left; (left = wake - scsynthNow()) > 0 && generation == gGeneration;)
				std::this_thread::sleep_for(std::chrono::duration<double>(std::min(left, .05)));
			if (generation != gGeneration) break;
			// /n_query fails once the main synth has ended, finite or freed
			if (!scsynthSendSync({oscMessage("/n_query", {{OscArg::Int, mainNode, ""}})}, 1000).empty()) break;
			for (std::string const& m : loadWavetables(*th, out, *loaded)) scsynthSend(m);
			for (std::string const& m : loadSamples(*th, out, *loaded, true)) scsynthSend(m);
			scsynthSend(defRecv(def));
			scsynthSend(oscBundle(startTime + seconds, {newSynth(name, 0), defFree(name)}, true));
		}
	} catch (...) {
		post("ola : stopped; a voice failed\n");
	}
}

} // namespace

void playTextures(Thread& th, Arg channels, double startTime, std::shared_ptr<LoadedTables> loaded, int mainNode)
{
	int generation = gGeneration;
	for (P<Object> const& t : texturesOf(th, channels)) {
		auto copy = std::make_shared<Thread>(th);
		++gRunning;
		std::thread(schedule, copy, P<Texture>((Texture*)t()), startTime, generation, loaded, mainNode).detach();
	}
}

void stopTextures() { ++gGeneration; }

int runningTextures() { return gRunning; }

std::string nrtScore(Thread& th, Arg channels, const std::string& name, double seconds)
{
	auto add = [](std::string& out, std::string const& bundle) {
		uint32_t n = (uint32_t)bundle.size();
		for (int shift = 24; shift >= 0; shift -= 8) out += (char)(uint8_t)(n >> shift);
		out += bundle;
	};
	std::string out;
	LoadedTables loaded;
	std::vector<std::string> start = loadWavetables(th, channels, loaded);
	for (std::string const& m : loadSamples(th, channels, loaded, false)) start.push_back(m);
	start.push_back(defRecv(compileSynthDef(th, channels, name)));
	add(out, oscBundle(0., start, false));
	add(out, oscBundle(0., {newSynth(name, 1)}, false));
	// Bundles must be in time order; voices of several textures are merged. Voices that compile
	// alike share a def, which stays loaded, so a score sends each def once.
	std::vector<std::pair<double, std::string>> events;
	std::map<std::string, std::string> names;  // def compiled as "v": its name in the score
	for (P<Object> const& t : texturesOf(th, channels)) {
		Voices voices(P<Texture>((Texture*)t()));
		double at;
		V voice;
		while (voices.next(th, at, voice) && at < seconds) {
			std::vector<std::string> msgs = loadWavetables(th, voice, loaded);
			for (std::string const& m : loadSamples(th, voice, loaded, false)) msgs.push_back(m);
			std::string& vname = names[voices.compile(th, voice, "v")];
			if (vname.empty()) {
				vname = voiceName();
				msgs.push_back(defRecv(voices.compile(th, voice, vname)));
			}
			if (!msgs.empty()) events.push_back({at, oscBundle(at, msgs, false)});
			events.push_back({at, oscBundle(at, {newSynth(vname, 0)}, false)});
		}
	}
	std::stable_sort(events.begin(), events.end(), [](auto& a, auto& b) { return a.first < b.first; });
	for (auto& e : events) add(out, e.second);
	add(out, oscBundle(seconds, {oscMessage("/c_set", {{OscArg::Int, 0, ""}, {OscArg::Int, 0, ""}})}, false));
	return out;
}

////////////////////////////////////////////////////////////////////////////////

static void ola_(Thread& th, Prim* prim)
{
	int64_t numChannels = th.popInt("ola : numChannels");
	double rate = th.popFloat("ola : rate");
	V hops = th.pop();
	V sounds = th.pop();
	// bus.audio takes buses from the top of the range down; ola's stay below them.
	int lastBus = std::min(kLastBus, audioBusFloor());
	if (numChannels < 1 || numChannels > lastBus - kFirstBus) throw errOutOfRange;
	if (rate <= 0.) {
		post("ola : rate must be positive\n");
		throw errOutOfRange;
	}
	if (gNextBus + numChannels > lastBus) gNextBus = kFirstBus;
	int bus = gNextBus;
	gNextBus += (int)numChannels;
	V in = makeUGen("In", rateAR, {(double)bus}, (int)numChannels, 0, false, true);
	P<UGenNode> node = isSig(in) ? ((Sig*)in.o())->node : ((Sig*)((List*)in.o())->at(0).o())->node;
	node->texture = new Texture(sounds, hops, rate, (int)numChannels, bus);
	if (numChannels == 1) {
		P<List> list = new List(itemTypeV, 1);
		list->add(in);
		in = list;
	}
	th.push(in);
}

void AddTextureOps()
{
	SapfWords sapf;
	vm.def("ola", 4, 1, ola_, "(sounds hops rate numChannels --> out) overlap add. This is the basic operator for polyphony. Each sound is a synth on a private bus; play starts them.");
}
