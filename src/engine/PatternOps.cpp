//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// stac's event streams (docs/dev/stac.md, Patterns): pbind makes a lazy list of Forms; play
// schedules one synth per event on a clock, pdef replaces a named player at the next beat, ndef
// crossfades a named sound, score writes events to an NRT command file. An event becomes synths as
// SC's default event does: pitch from freq, midinote or degree; amp .1; dur 1 beat; legato .8;
// a gate released after dur * legato when the def has one; 'rest; a list value plays a chord.

#include "Patterns.hpp"
#include "Def.hpp"
#include "Env.hpp"
#include "Graph.hpp"
#include "Namespace.hpp"
#include "Samples.hpp"
#include "Texture.hpp"
#include "Wavetables.hpp"
#include "symbol.hpp"
#include <atomic>
#include <chrono>
#include <cmath>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

namespace {

//////////////////////////////////////////////////////////////////////////////// pbind

// One Form per event: each field's next value. A list advances; anything else repeats.
struct Pbind : Gen
{
	P<TableMap> mMap;
	std::vector<VIn> mIns;

	Pbind(Thread& th, P<TableMap> const& map, std::vector<V> const& values, std::vector<bool> const& constant,
		bool finite)
		: Gen(th, itemTypeV, finite), mMap(map)
	{
		for (size_t i = 0; i < values.size(); ++i) {
			mIns.emplace_back(values[i]);
			if (constant[i]) mIns.back().setConstant(values[i]);
		}
	}

	virtual const char* TypeName() const override { return "Pbind"; }

	virtual void pull(Thread& th) override
	{
		int framesToFill = mBlockSize;
		V* out = mOut->fulfill(framesToFill);
		while (framesToFill) {
			P<Table> table = new Table(mMap);
			for (size_t i = 0; i < mIns.size(); ++i) {
				V v;
				if (mIns[i].one(th, v)) {
					setDone();
					produce(framesToFill);
					return;
				}
				table->put(i, v);
			}
			*out++ = V(new Form(table));
			--framesToFill;
		}
		produce(framesToFill);
	}
};

void pbind_(Thread& th, Prim* prim)
{
	V fields = th.pop();
	if (!fields.isForm()) wrongType("pbind : fields", "Form", fields);
	auto pairs = formPairs(th, (Form*)fields.o());
	P<TableMap> map = new TableMap(pairs.size());
	std::vector<V> values;
	std::vector<bool> constant;
	bool finite = false;
	for (size_t i = 0; i < pairs.size(); ++i) {
		V key(getsym(pairs[i].first.c_str()));
		map->put(i, key, key.Hash());
		Arg v = pairs[i].second;
		if (isSig(v)) {
			post("pbind : %s is a UGen; an event carries values, not graphs\n", pairs[i].first.c_str());
			throw errWrongType;
		}
		// A scale is a list; it does not advance.
		bool fixed = pairs[i].first == "scale";
		finite |= !fixed && v.isVList() && v.isFinite();
		values.push_back(v);
		constant.push_back(fixed);
	}
	th.push(new List(new Pbind(th, map, values, constant, finite)));
}

//////////////////////////////////////////////////////////////////////////////// pitch

// SC's Scale tables.
const std::map<std::string, std::vector<double>>& scales()
{
	static const std::map<std::string, std::vector<double>> s = {
		{"major", {0, 2, 4, 5, 7, 9, 11}}, {"ionian", {0, 2, 4, 5, 7, 9, 11}},
		{"minor", {0, 2, 3, 5, 7, 8, 10}}, {"aeolian", {0, 2, 3, 5, 7, 8, 10}},
		{"dorian", {0, 2, 3, 5, 7, 9, 10}}, {"phrygian", {0, 1, 3, 5, 7, 8, 10}},
		{"lydian", {0, 2, 4, 6, 7, 9, 11}}, {"mixolydian", {0, 2, 4, 5, 7, 9, 10}},
		{"locrian", {0, 1, 3, 5, 6, 8, 10}}, {"harmonicMinor", {0, 2, 3, 5, 7, 8, 11}},
		{"melodicMinor", {0, 2, 3, 5, 7, 9, 11}}, {"majorPentatonic", {0, 2, 4, 7, 9}},
		{"minorPentatonic", {0, 3, 5, 7, 10}}, {"whole", {0, 2, 4, 6, 8, 10}},
		{"chromatic", {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}}};
	return s;
}

std::vector<double> scaleOf(Thread& th, Arg v)
{
	if (const char* name = symbolOf(v)) {
		auto it = scales().find(name);
		if (it == scales().end()) {
			post("event : no scale named '%s\n", name);
			throw errNotFound;
		}
		return it->second;
	}
	if (v.isVList() && v.isFinite()) {
		P<List> list = ((List*)v.o())->pack(th);
		std::vector<double> out;
		for (int64_t i = 0; i < list->mArray->size(); ++i) out.push_back(list->mArray->at(i).asFloat());
		if (!out.empty()) return out;
	}
	wrongType("event : scale", "scale name or list of steps", v);
}

// SC's SimpleNumber.degreeToKey and Scale.performDegreeToKey: a fraction of .1 is a semitone.
double degreeToKey(double degree, std::vector<double> const& scale)
{
	double rounded = std::round(degree);
	double accidental = (degree - rounded) * 10.;
	int64_t d = (int64_t)rounded, n = (int64_t)scale.size();
	int64_t octave = d >= 0 ? d / n : -((-d + n - 1) / n);
	return 12. * octave + scale[(size_t)(d - octave * n)] + accidental;
}

double midicps(double note) { return 440. * std::pow(2., (note - 69.) / 12.); }

//////////////////////////////////////////////////////////////////////////////// events

std::atomic<int64_t> gEventDefs{0};

struct Timed {
	double beats;
	std::string msg;
};

// Turns an event stream into OSC messages timed in beats, one event at a time.
struct Events {
	Thread& th;
	VIn events;
	double beat = 0.; // where the next event starts
	std::vector<Def*> defs; // each def the events have used, in order
	// Messages to send before the next event's: the wavetables, sound files and defs of function
	// instruments. absolute: sound file paths as live scsynth reads them, not as a score's.
	std::vector<std::string> loads;
	bool absolute;
	LoadedTables loaded;
	std::map<std::string, std::string> funDefs; // a function's def compiled as "v": its name

	Events(Thread& inTh, Arg stream, bool live) : th(inTh), absolute(live) { events.set(stream); }

	Def* defNamed(Arg v)
	{
		if (Def* d = asDef(v)) return d;
		if (const char* name = symbolOf(v)) {
			V value;
			if (th.mWorkspace->get(th, v, value))
				if (Def* d = asDef(value)) return d;
			post("event : no def named %s\n", name);
			throw errNotFound;
		}
		wrongType("event : instrument", "def name, Def or function", v);
	}

	static double real(Arg v, const char* key)
	{
		if (!v.isReal()) wrongType((std::string("event : ") + key).c_str(), "Real", v);
		return v.f;
	}

	// The next event's messages, appended to out with times from the stream's start. False at its end.
	bool next(std::vector<Timed>& out)
	{
		V ev;
		if (events.one(th, ev)) return false;
		if (!ev.isForm()) wrongType("event", "Form", ev);
		std::map<std::string, V> f;
		for (auto& [k, v] : formPairs(th, (Form*)ev.o())) f[k] = v;
		auto has = [&](const char* k) { return f.count(k) > 0; };

		double dur = has("dur") ? real(f["dur"], "dur") : 1.;
		double start = beat;
		beat += dur;
		for (auto& [k, v] : f)
			if (const char* s = symbolOf(v); s && !strcmp(s, "rest")) return true;
		if (!has("instrument")) {
			post("event : needs an :instrument\n");
			throw errNotFound;
		}
		V fun = f["instrument"];
		Def* def = fun.isFun() ? nullptr : defNamed(fun);
		if (def && std::find(defs.begin(), defs.end(), def) == defs.end()) defs.push_back(def);
		double legato = has("legato") ? real(f["legato"], "legato") : .8;
		double sustain = has("sustain") ? real(f["sustain"], "sustain") : dur * legato;
		auto param = [&](const char* p) { return std::find(def->mParams.begin(), def->mParams.end(), p) != def->mParams.end(); };

		// A list value plays a chord: one synth per element, shorter lists wrapping.
		int64_t notes = 1;
		std::map<std::string, P<List>> lists;
		for (auto& [k, v] : f) {
			if (k == "scale" || !v.isVList() || !v.isFinite()) continue;
			P<List> l = ((List*)v.o())->pack(th);
			if (l->mArray->size() == 0) return true;
			lists[k] = l;
			notes = std::max(notes, l->mArray->size());
		}
		for (int64_t n = 0; n < notes; ++n) {
			auto at = [&](std::string const& k) { return lists.count(k) ? lists[k]->mArray->wrapAt(n) : f[k]; };
			auto freq = [&] {
				if (has("freq")) return real(at("freq"), "freq");
				double note;
				if (has("midinote")) {
					note = real(at("midinote"), "midinote");
				} else {
					std::vector<double> scale = scaleOf(th, has("scale") ? f["scale"] : V(getsym("major")));
					double degree = has("degree") ? real(at("degree"), "degree") : 0.;
					double root = has("root") ? real(at("root"), "root") : 0.;
					double octave = has("octave") ? real(at("octave"), "octave") : 5.;
					note = degreeToKey(degree, scale) + root + (octave - 5.) * 12. + 60.;
				}
				return midicps(note);
			};
			if (!def) {
				int bus = 0;
				if (has("out")) {
					V x = at("out");
					if (!x.isReal() && !asResource(x)) wrongType("event : out", "Real or Bus", x);
					bus = (int)x.asFloat();
				}
				out.push_back({start, synthOf(fun, bus, [&](std::string const& k) -> V {
					if (k == "freq") return freq();
					if (k == "amp" && !has("amp")) return .1;
					if (k == "dur") return dur;
					if (k == "legato") return legato;
					if (k == "sustain") return sustain;
					if (!has(k.c_str())) {
						post("event : the instrument's argument %s has no field\n", k.c_str());
						throw errNotFound;
					}
					return at(k);
				})});
				continue;
			}
			std::vector<OscArg> controls;
			auto send = [&](std::string const& name, double value) {
				controls.push_back({OscArg::String, 0, name});
				controls.push_back({OscArg::Float, 0, "", (float)value});
			};
			for (auto& [k, v] : f) {
				if (!param(k.c_str())) continue;
				V x = at(k);
				if (!x.isReal() && !asResource(x)) wrongType(("event : " + k).c_str(), "Real, Bus, Buffer or Node", x);
				send(k, x.asFloat());
			}
			if (param("freq") && !has("freq")) send("freq", freq());
			if (param("amp") && !has("amp")) send("amp", .1);
			int id = newNodeID();
			std::vector<OscArg> args = {{OscArg::String, 0, def->mName}, {OscArg::Int, id, ""}, {OscArg::Int, 0, ""},
				{OscArg::Int, 0, ""}};
			args.insert(args.end(), controls.begin(), controls.end());
			out.push_back({start, oscMessage("/s_new", args)});
			if (param("gate"))
				out.push_back({start + sustain, oscMessage("/n_set", {{OscArg::Int, id, ""},
					{OscArg::String, 0, "gate"}, {OscArg::Float, 0, "", 0.f}})});
		}
		return true;
	}

	// A function instrument's synth, writing to bus: fun called with each argument's value, as ola calls a voice.
	// Synths that compile alike share a def. Their channels must end, as an ola voice's do.
	template <class Value>
	std::string synthOf(V fun, int bus, Value value)
	{
		V channels;
		{
			SaveStack ss(th);
			for (P<String> const& name : ((Fun*)fun.o())->mDef()->mArgNames) th.push(value(name->s));
			fun.apply(th);
			channels = th.pop();
		}
		if (!isSig(channels) && !(channels.isVList() && channels.isFinite()))
			wrongType("event : instrument's result", "UGen or finite list of channels", channels);
		if (!texturesOf(th, channels).empty()) {
			post("event : an instrument cannot contain an ola\n");
			throw errFailed;
		}
		for (std::string const& m : loadWavetables(th, channels, loaded)) loads.push_back(m);
		for (std::string const& m : loadSamples(th, channels, loaded, absolute)) loads.push_back(m);
		std::string& name = funDefs[compileSynthDef(th, channels, "v", bus)];
		if (name.empty()) {
			name = "sapf-event-" + std::to_string(++gEventDefs);
			loads.push_back(oscMessage("/d_recv", {{OscArg::Blob, 0, compileSynthDef(th, channels, name, bus)}}));
		}
		return oscMessage("/s_new", {{OscArg::String, 0, name}, {OscArg::Int, newNodeID(), ""}, {OscArg::Int, 0, ""},
			{OscArg::Int, 0, ""}});
	}
};

//////////////////////////////////////////////////////////////////////////////// clocks

// SC's TempoClock: beats advance at tempo beats per second from (beats0, seconds0), in server time.
// A tempo change rebases at now, so the beats already passed keep their times.
struct ClockState {
	std::mutex mutex;
	double beats0 = 0., seconds0 = scsynthNow(), tempo = 1.;

	double secondsAt(double beats)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return seconds0 + (beats - beats0) / tempo;
	}
	double beatsAt(double seconds)
	{
		std::lock_guard<std::mutex> lock(mutex);
		return beats0 + (seconds - seconds0) * tempo;
	}
	void setTempo(double t)
	{
		double now = scsynthNow(), b = beatsAt(now);
		std::lock_guard<std::mutex> lock(mutex);
		beats0 = b, seconds0 = now, tempo = t;
	}
};

std::shared_ptr<ClockState> defaultClock()
{
	static std::shared_ptr<ClockState> clock = std::make_shared<ClockState>();
	return clock;
}

double popTempo(Thread& th, const char* msg)
{
	double t = th.popFloat(msg);
	if (!(t > 0.) || !std::isfinite(t)) {
		post("%s : above 0\n", msg);
		throw errOutOfRange;
	}
	return t;
}

//////////////////////////////////////////////////////////////////////////////// play

std::atomic<int> gGeneration{0};
const double kLookahead = .2; // as ola's scheduler

// What a running stream checks between events: .stop, and the beat pdef replaces it at.
struct PlayState {
	int generation = gGeneration;
	std::atomic<bool> stopped{false};
	std::atomic<double> stopBeat{std::numeric_limits<double>::infinity()};

	bool live() const { return generation == gGeneration && !stopped; }
};

class Player : public Object
{
public:
	std::shared_ptr<PlayState> mState;

	explicit Player(std::shared_ptr<PlayState> state) : mState(std::move(state)) {}

	virtual const char* TypeName() const override { return "Player"; }

	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (!name || strcmp(name, "stop")) return false;
		mState->stopped = true;
		ioValue = noValue();
		return true;
	}
};

void run(std::shared_ptr<Thread> th, V stream, std::shared_ptr<ClockState> clock, double startBeat,
	std::shared_ptr<PlayState> state)
{
	try {
		Events events(*th, stream, true);
		std::vector<Timed> msgs;
		size_t sentDefs = 0;
		for (double beat = startBeat; state->live() && beat < state->stopBeat && events.next(msgs);
			 beat = startBeat + events.beat) {
			for (; sentDefs < events.defs.size(); ++sentDefs) {
				Def* d = events.defs[sentDefs];
				if (!d->mSent) checkServer(scsynthSendSync({d->recv()}), d->mName.c_str());
				d->mSent = true;
			}
			if (!events.loads.empty()) checkServer(scsynthSendSync(events.loads), "play");
			events.loads.clear();
			// wait until the event is due, less the lookahead; the tempo may change meanwhile
			for (double left; (left = clock->secondsAt(beat) - kLookahead - scsynthNow()) > 0 && state->live();)
				std::this_thread::sleep_for(std::chrono::duration<double>(std::min(left, .05)));
			if (!state->live() || beat >= state->stopBeat) break;
			for (Timed const& m : msgs)
				scsynthSend(oscBundle(clock->secondsAt(startBeat + m.beats), {m.msg}, true));
			msgs.clear();
		}
	} catch (...) {
		post("play : stopped; an event failed\n");
	}
}

// Starts stream at startBeat, or as soon as the lookahead allows when it is nan.
P<Player> start(Thread& th, Arg stream, std::shared_ptr<ClockState> clock, double startBeat)
{
	if (std::isnan(startBeat)) startBeat = clock->beatsAt(scsynthNow() + kLookahead);
	auto state = std::make_shared<PlayState>();
	std::thread(run, std::make_shared<Thread>(th), V(stream), clock, startBeat, state).detach();
	return new Player(state);
}

V popEvents(Thread& th, const char* msg)
{
	V stream = th.pop();
	if (!isEventStream(th, stream)) wrongType(msg, "event stream", stream);
	return stream;
}

class Clock : public Object
{
public:
	std::shared_ptr<ClockState> mState = std::make_shared<ClockState>();

	virtual const char* TypeName() const override { return "Clock"; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		std::lock_guard<std::mutex> lock(mState->mutex);
		char s[64];
		snprintf(s, sizeof(s), "Clock (%g beats per second)", mState->tempo);
		out += s;
	}

	// play starts an event stream on this clock; set changes its tempo.
	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (!name) return false;
		if (!strcmp(name, "play")) {
			V stream = popEvents(th, "play : events");
			ioValue = V(start(th, stream, mState, NAN));
			return true;
		}
		if (strcmp(name, "set")) return false;
		V form = th.pop();
		if (!form.isForm()) wrongType("set : fields", "Form", form);
		for (auto& [k, v] : formPairs(th, (Form*)form.o())) {
			if (k != "tempo") {
				post("set : a clock has no field %s\n", k.c_str());
				throw errNotFound;
			}
			th.push(v);
			mState->setTempo(popTempo(th, "set : tempo"));
		}
		ioValue = noValue();
		return true;
	}
};

void new_(Thread& th, Prim* prim)
{
	P<Clock> clock = new Clock();
	clock->mState->tempo = popTempo(th, "clock.new : tempo");
	th.push(V(clock));
}

// pdef's players by name. A name's Player is kept; each pdef points it at the new stream.
std::map<std::string, P<Player>> gPdefs;

void pdef_(Thread& th, Prim* prim)
{
	V name = th.pop();
	const char* s = symbolOf(name);
	if (!s) wrongType("pdef : name", "Symbol", name);
	V stream = popEvents(th, "pdef : events");
	auto clock = defaultClock();
	// SC's Pdef quant of 1: the old stream stops and the new one starts on the next whole beat.
	double next = std::ceil(clock->beatsAt(scsynthNow() + kLookahead));
	P<Player>& player = gPdefs[s];
	if (player) player->mState->stopBeat = next;
	P<Player> started = start(th, stream, clock, next);
	if (player) player->mState = started->mState;
	else player = started;
	th.push(V(player));
}

//////////////////////////////////////////////////////////////////////////////// ndef

// A named sound. Each ndef starts a synth that fades in and releases the one before it.
class Ndef : public Object
{
public:
	std::string mName;
	int mNode = -1; // the current synth, or -1
	int mDefs = 0;
	double mFadeTime = .02; // SC's NodeProxy default

	explicit Ndef(std::string name) : mName(std::move(name)) {}

	virtual const char* TypeName() const override { return "Ndef"; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += "Ndef " + mName + (mNode < 0 ? " (stopped)" : " (node " + std::to_string(mNode) + ")");
	}

	// stop fades the sound out; {:fadeTime t} nd.set sets the fade of this synth and later ones.
	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (!name) return false;
		if (!strcmp(name, "stop")) {
			// a plain send: scsynth answers /fail when the synth has already ended
			if (mNode >= 0)
				scsynthSend(oscMessage("/n_set", {{OscArg::Int, mNode, ""}, {OscArg::String, 0, "gate"},
					{OscArg::Float, 0, "", 0.f}}));
			mNode = -1;
		} else if (!strcmp(name, "set")) {
			V form = th.pop();
			if (!form.isForm()) wrongType("set : fields", "Form", form);
			for (auto& [k, v] : formPairs(th, (Form*)form.o())) {
				if (k != "fadeTime") {
					post("set : an ndef has no field %s\n", k.c_str());
					throw errNotFound;
				}
				if (!v.isReal() || v.f < 0.) wrongType("set : fadeTime", "Real of 0 or more", v);
				mFadeTime = v.f;
				if (mNode >= 0)
					scsynthSend(oscMessage("/n_set", {{OscArg::Int, mNode, ""}, {OscArg::String, 0, "fadeTime"},
						{OscArg::Float, 0, "", (float)mFadeTime}}));
			}
		} else {
			return false;
		}
		ioValue = noValue();
		return true;
	}
};

std::map<std::string, P<Ndef>> gNdefs;

// SC's ProxySynthDef: the channels times EnvGate, an EnvGen from 0 to 1 and back over fadeTime with
// a 'sin curve, released by gate and freeing the synth.
std::string fadeDef(Thread& th, Arg channels, std::string const& name)
{
	DefParts parts;
	V gate = makeParam("gate", 1., paramKR, 0), fade = makeParam("fadeTime", .02, paramKR, 1);
	parts.params = {gate, fade, makeParam("out", 0., paramKR, 2)};
	parts.outBus = parts.params[2];
	P<Env> env = new Env();
	env->levels = {0., 1., 0.};
	env->times = {1., 1.};
	env->curves = {V(getsym("sin"))};
	env->releaseNode = 1.;
	std::vector<V> inputs = {gate, 1., 0., fade, 2.};
	for (Arg x : env->envGenArray("ndef")) inputs.push_back(x);
	V envGate = makeUGen("EnvGen", rateKR, inputs, 1, 0, true, true);
	std::vector<V> flat;
	flattenChannels(th, channels, flat);
	P<List> faded = new List(itemTypeV, (int64_t)flat.size());
	for (Arg c : flat) faded->add(graphBinary(th, opMul, c, envGate));
	return compileDef(th, V(faded), name, parts);
}

void ndef_(Thread& th, Prim* prim)
{
	V name = th.pop();
	const char* s = symbolOf(name);
	if (!s) wrongType("ndef : name", "Symbol", name);
	V channels = th.pop();
	if (isEventStream(th, channels)) wrongType("ndef : channels", "channels; pdef plays events", channels);
	P<Ndef>& nd = gNdefs[s];
	if (!nd) nd = new Ndef(s);
	std::string defName = "sapf-ndef-" + nd->mName + "-" + std::to_string(++nd->mDefs);
	std::string def = fadeDef(th, channels, defName);
	auto loaded = std::make_shared<LoadedTables>();
	std::vector<std::string> packets = loadWavetables(th, channels, *loaded);
	for (std::string const& m : loadSamples(th, channels, *loaded, true)) packets.push_back(m);
	packets.push_back(oscMessage("/d_recv", {{OscArg::Blob, 0, def}}));
	checkServer(scsynthSendSync(packets), "ndef");
	// The new synth starts as the old one is released, so their fades overlap.
	int node = newNodeID();
	double at = scsynthNow() + kLookahead;
	std::vector<std::string> msgs = {oscMessage("/s_new", {{OscArg::String, 0, defName}, {OscArg::Int, node, ""},
		{OscArg::Int, 0, ""}, {OscArg::Int, 0, ""}, {OscArg::String, 0, "fadeTime"},
		{OscArg::Float, 0, "", (float)nd->mFadeTime}})};
	if (nd->mNode >= 0)
		msgs.push_back(oscMessage("/n_set", {{OscArg::Int, nd->mNode, ""}, {OscArg::String, 0, "gate"},
			{OscArg::Float, 0, "", 0.f}}));
	scsynthSend(oscBundle(at, msgs, true));
	nd->mNode = node;
	playTextures(th, channels, at, loaded, node);
	th.push(V(nd));
}

//////////////////////////////////////////////////////////////////////////////// score

// The NRT command file for an event stream's first seconds, at 1 beat per second.
std::string eventScore(Thread& th, Arg stream, double seconds)
{
	std::string score;
	auto add = [&](std::string const& bundle) {
		uint32_t n = (uint32_t)bundle.size();
		for (int shift = 24; shift >= 0; shift -= 8) score += (char)(uint8_t)(n >> shift);
		score += bundle;
	};
	Events events(th, stream, false);
	std::vector<Timed> msgs;
	for (int64_t count = 0; events.beat < seconds && events.next(msgs); ++count) {
		if (count > 1000000) {
			post("score : more than a million events before %g seconds\n", seconds);
			throw errOutOfRange;
		}
	}
	std::vector<std::string> recvs = events.loads;
	for (Def* d : events.defs) recvs.push_back(d->recv());
	add(oscBundle(0., recvs, false));
	std::stable_sort(msgs.begin(), msgs.end(), [](Timed const& a, Timed const& b) { return a.beats < b.beats; });
	for (Timed const& m : msgs)
		if (m.beats < seconds) add(oscBundle(m.beats, {m.msg}, false));
	add(oscBundle(seconds, {oscMessage("/c_set", {{OscArg::Int, 0, ""}, {OscArg::Int, 0, ""}})}, false));
	return score;
}

// (item seconds name): the parts score and render share. seconds must be above 0.
struct ScoreArgs {
	std::string name;
	double seconds;
	V item;
	bool events;

	ScoreArgs(Thread& th, const char* word)
	{
		V n = th.pop();
		if (!n.isString()) wrongType((std::string(word) + " : name").c_str(), "String", n);
		name = ((String*)n.o())->s;
		seconds = th.popFloat((std::string(word) + " : seconds").c_str());
		if (!(seconds > 0.) || !std::isfinite(seconds)) {
			post("%s : seconds : above 0\n", word);
			throw errOutOfRange;
		}
		item = th.pop();
		events = isEventStream(th, item);
	}

	std::string score(Thread& th) { return events ? eventScore(th, item, seconds) : nrtScore(th, item, name, seconds); }
};

void score_(Thread& th, Prim* prim)
{
	ScoreArgs args(th, "score");
	std::string score = args.score(th);
	const char* dir = getenv("SAPF_SYNTHDEFS");
	std::string path = ((dir && *dir) ? std::string(dir) + "/" : std::string()) + args.name + ".osc";
	std::ofstream f(path, std::ios::binary);
	if (!f || !f.write(score.data(), (std::streamsize)score.size())) {
		post("score : could not write '%s'\n", path.c_str());
		throw errFailed;
	}
	post("wrote '%s'\n", path.c_str());
}

// Events render to 2 channels, SC's default numOutputBusChannels for Score.render.
void render_(Thread& th, Prim* prim)
{
	ScoreArgs args(th, "render");
	int channels = args.events ? 2 : channelCount(th, args.item);
	renderScore(th, args.score(th), args.name, channels, args.seconds, "render");
}

} // namespace

bool isEventStream(Thread& th, Arg v)
{
	if (!v.isVList()) return false;
	VIn in;
	in.set(v);
	V first;
	return !in.one(th, first) && first.isForm();
}

void playEventStream(Thread& th, Arg stream) { th.push(V(start(th, stream, defaultClock(), NAN))); }

void stopEventStreams()
{
	++gGeneration;
	for (auto& [name, nd] : gNdefs) nd->mNode = -1;
}

void AddPatternOps();
void AddPatternOps()
{
	vm.addBifHelp(std::string("\n*** patterns ***"));
	vm.def("pbind", 1, 1, pbind_,
		"(fields --> events) a lazy list of events, one Form per event: each list field's next value, other "
		"fields and :scale repeating. Ends with the shortest finite field. play starts one synth per event.");
	vm.def("score", 3, 0, score_,
		"(item seconds name -->) writes name.osc, an NRT command file of item's first seconds. item is an event "
		"stream, or channels as >nrt takes. Events are timed at 1 beat per second.");
	vm.def("render", 3, 0, render_,
		"(item seconds name -->) renders item's first seconds to name.wav in $SAPF_RECORDINGS with the embedded "
		"scsynth. Channels keep their count; events render to 2 channels at 1 beat per second.");
	vm.def("pdef", 2, 1, pdef_,
		"(events name --> player) plays events on the default clock from the next beat. Run again with the same "
		"name, it replaces the stream at the next beat and pushes the same player.");

	vm.def("ndef", 2, 1, ndef_,
		"(channels name --> ndef) plays channels, fading in over fadeTime (.02 s). Run again with the same name, "
		"it fades the new sound in and the old one out, and pushes the same ndef. nd.stop fades it out; "
		"{:fadeTime t} nd.set sets the fade.");

	vm.addBifHelp(std::string("\n*** clock ***"));
	P<Namespace> clock = new Namespace("clock");
	clock->addPrim("new", 1, new_,
		"(tempo --> clock) a clock at tempo beats per second. events clk.play plays on it; {:tempo t} clk.set "
		"changes its tempo.");
	vm.def("clock", V(clock));
	vm.addBifHelp("clock", nullptr, "(--> namespace) clocks. The default clock runs at 1 beat per second.");
}
