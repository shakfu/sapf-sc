//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// sapf UGen primitives that build SuperCollider UGen graphs. Arguments keep sapf's order
// and units; each primitive converts them to the SC UGen's wire order and units.
// docs/ugens.md lists the mappings.

#include "Graph.hpp"
#include "Scsynth.hpp"
#include "MathFuns.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>

static V mul(Thread& th, Arg a, Arg b) { return graphBinary(th, opMul, a, b); }
static V add(Thread& th, Arg a, Arg b) { return graphBinary(th, opAdd, a, b); }

// sapf phases are in cycles; SC's SinOsc phase is in radians.
static V cyclesToRadians(Thread& th, Arg phase) { return mul(th, phase, kTwoPi); }

// Bipolar output from a unipolar one: x*2 - 1.
static V unibi(Thread& th, Arg x) { return graphBinary(th, opSub, mul(th, x, 2.), 1.); }

#define POP(NAME, PRIM) V NAME = popGraphIn(th, PRIM " : " #NAME)

////////////////////////////////////////////////////////////////////////////////
// oscillators

static void sinosc_(Thread& th, Prim* prim)
{
	POP(phase, "sinosc");
	POP(freq, "sinosc");
	th.push(makeUGen("SinOsc", rateAR, {freq, cyclesToRadians(th, phase)}));
}

// SC's LFSaw, LFTri and LFPulse take an initial phase. sapf's phase is in cycles; LFSaw's and
// LFTri's are in half cycles, LFPulse's in cycles.
static void lfsaw_(Thread& th, Prim* prim)
{
	V phase = th.popFloat("lfsaw : phase");
	POP(freq, "lfsaw");
	th.push(makeUGen("LFSaw", rateAR, {freq, sc_wrap(phase.f * 2., 0., 2.)}));
}

static void lftri_(Thread& th, Prim* prim)
{
	V phase = th.popFloat("lftri : phase");
	POP(freq, "lftri");
	th.push(makeUGen("LFTri", rateAR, {freq, sc_wrap(phase.f * 2., 0., 2.)}));
}

static void lfpulse_(Thread& th, Prim* prim)
{
	POP(duty, "lfpulse");
	V phase = th.popFloat("lfpulse : phase");
	POP(freq, "lfpulse");
	th.push(makeUGen("LFPulse", rateAR, {freq, sc_wrap(phase.f, 0., 1.), duty}));
}

// sapf's bipolar pulse is duty when high and duty - 1 when low.
static void lfpulseb_(Thread& th, Prim* prim)
{
	POP(duty, "lfpulseb");
	V phase = th.popFloat("lfpulseb : phase");
	POP(freq, "lfpulseb");
	V pulse = makeUGen("LFPulse", rateAR, {freq, sc_wrap(phase.f, 0., 1.), duty});
	th.push(add(th, pulse, graphBinary(th, opSub, duty, 1.)));
}

static void lfsquare_(Thread& th, Prim* prim)
{
	V phase = th.popFloat("lfsquare : phase");
	POP(freq, "lfsquare");
	th.push(unibi(th, makeUGen("LFPulse", rateAR, {freq, sc_wrap(phase.f, 0., 1.), .5})));
}

static void impulse_(Thread& th, Prim* prim)
{
	POP(phase, "impulse");
	POP(freq, "impulse");
	th.push(makeUGen("Impulse", rateAR, {freq, phase}));
}

////////////////////////////////////////////////////////////////////////////////
// noise

static void white_(Thread& th, Prim* prim)
{
	POP(amp, "white");
	th.push(mul(th, makeUGen("WhiteNoise", rateAR, {}), amp));
}

static void pink_(Thread& th, Prim* prim)
{
	POP(amp, "pink");
	th.push(mul(th, makeUGen("PinkNoise", rateAR, {}), amp));
}

static void brown_(Thread& th, Prim* prim)
{
	POP(amp, "brown");
	th.push(mul(th, makeUGen("BrownNoise", rateAR, {}), amp));
}

static void dust_(Thread& th, Prim* prim)
{
	POP(amp, "dust");
	POP(density, "dust");
	th.push(mul(th, makeUGen("Dust", rateAR, {density}), amp));
}

static void dust2_(Thread& th, Prim* prim)
{
	POP(amp, "dust2");
	POP(density, "dust2");
	th.push(mul(th, makeUGen("Dust2", rateAR, {density}), amp));
}

static void lfnoise0_(Thread& th, Prim* prim)
{
	POP(freq, "lfnoise0");
	th.push(makeUGen("LFNoise0", rateAR, {freq}));
}

static void lfnoise1_(Thread& th, Prim* prim)
{
	POP(freq, "lfnoise1");
	th.push(makeUGen("LFNoise1", rateAR, {freq}));
}

////////////////////////////////////////////////////////////////////////////////
// filters

static void lag_(Thread& th, Prim* prim)
{
	POP(time, "lag");
	POP(in, "lag");
	th.push(makeUGen("Lag", rateAR, {in, time}));
}

static void lagud_(Thread& th, Prim* prim)
{
	POP(down, "lagud");
	POP(up, "lagud");
	POP(in, "lagud");
	th.push(makeUGen("LagUD", rateAR, {in, up, down}));
}

static void lpf_(Thread& th, Prim* prim)
{
	POP(freq, "lpf");
	POP(in, "lpf");
	th.push(makeUGen("LPF", rateAR, {in, freq}));
}

static void hpf_(Thread& th, Prim* prim)
{
	POP(freq, "hpf");
	POP(in, "hpf");
	th.push(makeUGen("HPF", rateAR, {in, freq}));
}

static void rlpf_(Thread& th, Prim* prim)
{
	POP(rq, "rlpf");
	POP(freq, "rlpf");
	POP(in, "rlpf");
	th.push(makeUGen("RLPF", rateAR, {in, freq, rq}));
}

static void rhpf_(Thread& th, Prim* prim)
{
	POP(rq, "rhpf");
	POP(freq, "rhpf");
	POP(in, "rhpf");
	th.push(makeUGen("RHPF", rateAR, {in, freq, rq}));
}

// sapf gives bandwidth in octaves, SC 1/Q: rq = 2^(bw/2) - 2^(-bw/2).
static V octavesToRQ(Thread& th, Arg bw)
{
	if (bw.isReal()) return exp2(bw.f * .5) - exp2(-bw.f * .5);
	V half = mul(th, bw, .5);
	return graphBinary(th, opSub, graphBinary(th, opPow, 2., half), graphBinary(th, opPow, 2., graphUnary(th, opNeg, half)));
}

static void bpf_(Thread& th, Prim* prim)
{
	POP(bw, "bpf");
	POP(freq, "bpf");
	POP(in, "bpf");
	th.push(makeUGen("BPF", rateAR, {in, freq, octavesToRQ(th, bw)}));
}

static void bsf_(Thread& th, Prim* prim)
{
	POP(bw, "bsf");
	POP(freq, "bsf");
	POP(in, "bsf");
	th.push(makeUGen("BRF", rateAR, {in, freq, octavesToRQ(th, bw)}));
}

// sapf's resonz has no gain normalization; SC's scales by 1 - R^2, R = 1 - w0*rq/2.
static void resonz_(Thread& th, Prim* prim)
{
	POP(rq, "resonz");
	POP(freq, "resonz");
	POP(in, "resonz");
	V w0 = mul(th, freq, makeUGen("RadiansPerSample", rateIR, {}));
	V R = graphBinary(th, opSub, 1., mul(th, mul(th, w0, rq), .5));
	V gain = graphUnary(th, opRecip, graphBinary(th, opSub, 1., graphUnary(th, opSquared, R)));
	th.push(mul(th, makeUGen("Resonz", rateAR, {in, freq, rq}), gain));
}

static void ringz_(Thread& th, Prim* prim)
{
	POP(time, "ringz");
	POP(freq, "ringz");
	POP(in, "ringz");
	th.push(makeUGen("Ringz", rateAR, {in, freq, time}));
}

static void formlet_(Thread& th, Prim* prim)
{
	POP(dcy, "formlet");
	POP(atk, "formlet");
	POP(freq, "formlet");
	POP(in, "formlet");
	th.push(mul(th, makeUGen("Formlet", rateAR, {in, freq, atk, dcy}), 2.)); // SC scales by .25, sapf .5
}

static void leakdc_(Thread& th, Prim* prim)
{
	POP(coef, "leakdc");
	POP(in, "leakdc");
	th.push(makeUGen("LeakDC", rateAR, {in, coef}));
}

static void decay_(Thread& th, Prim* prim)
{
	POP(time, "decay");
	POP(in, "decay");
	th.push(makeUGen("Decay", rateAR, {in, time}));
}

static void decay2_(Thread& th, Prim* prim)
{
	POP(dcy, "decay2");
	POP(atk, "decay2");
	POP(in, "decay2");
	th.push(makeUGen("Decay2", rateAR, {in, atk, dcy}));
}

////////////////////////////////////////////////////////////////////////////////
// delays. sapf: (in delay maxdelay [decay]). SC: (in maxdelay delay [decay]).

// As in sapf, a maxdelay of zero means the delay, which must then be a constant.
static double maxDelay(const char* name, double maxdelay, Arg delay)
{
	if (maxdelay != 0.) return maxdelay;
	if (delay.isReal()) return delay.f;
	post("%s : maxdelay is zero and delay is a UGen\n", name);
	throw errFailed;
}

static void delay(Thread& th, const char* name, const char* ugen)
{
	double maxdelay = th.popFloat(name);
	V delay = popGraphIn(th, name);
	V in = popGraphIn(th, name);
	th.push(makeUGen(ugen, rateAR, {in, maxDelay(name, maxdelay, delay), delay}));
}

// sapf's comb output is SC's times the feedback coefficient, 0.001^(delay/decay) with the
// sign of decay. sapf clamps the exponent to 2.
static V feedback(Thread& th, Arg delay, Arg decay)
{
	if (delay.isReal() && decay.isReal()) {
		double ratio = delay.f / decay.f;
		return sc_sgn(ratio) * pow(.001, std::min(fabs(ratio), 2.));
	}
	V ratio = graphBinary(th, opFDiv, delay, decay);
	V exponent = graphBinary(th, opMin, graphUnary(th, opAbs, ratio), 2.);
	return mul(th, graphUnary(th, opSign, ratio), graphBinary(th, opPow, .001, exponent));
}

static void comb(Thread& th, const char* name, const char* ugen)
{
	V decay = popGraphIn(th, name);
	double maxdelay = th.popFloat(name);
	V delay = popGraphIn(th, name);
	V in = popGraphIn(th, name);
	V comb = makeUGen(ugen, rateAR, {in, maxDelay(name, maxdelay, delay), delay, decay});
	th.push(mul(th, comb, feedback(th, delay, decay)));
}

static void allpass(Thread& th, const char* name, const char* ugen)
{
	V decay = popGraphIn(th, name);
	double maxdelay = th.popFloat(name);
	V delay = popGraphIn(th, name);
	V in = popGraphIn(th, name);
	th.push(makeUGen(ugen, rateAR, {in, maxDelay(name, maxdelay, delay), delay, decay}));
}

static void delayn_(Thread& th, Prim* prim) { delay(th, "delayn", "DelayN"); }
static void delayl_(Thread& th, Prim* prim) { delay(th, "delayl", "DelayL"); }
static void delayc_(Thread& th, Prim* prim) { delay(th, "delayc", "DelayC"); }
static void combn_(Thread& th, Prim* prim) { comb(th, "combn", "CombN"); }
static void combl_(Thread& th, Prim* prim) { comb(th, "combl", "CombL"); }
static void combc_(Thread& th, Prim* prim) { comb(th, "combc", "CombC"); }
static void alpasn_(Thread& th, Prim* prim) { allpass(th, "alpasn", "AllpassN"); }
static void alpasl_(Thread& th, Prim* prim) { allpass(th, "alpasl", "AllpassL"); }
static void alpasc_(Thread& th, Prim* prim) { allpass(th, "alpasc", "AllpassC"); }

////////////////////////////////////////////////////////////////////////////////
// panning

static void pan2_(Thread& th, Prim* prim)
{
	POP(pos, "pan2");
	POP(in, "pan2");
	th.push(makeUGen("Pan2", rateAR, {in, pos, 1.}, 2));
}

static void bal2_(Thread& th, Prim* prim)
{
	POP(pos, "bal2");
	POP(right, "bal2");
	POP(left, "bal2");
	th.push(makeUGen("Balance2", rateAR, {left, right, pos, 1.}, 2));
}

static void rot2_(Thread& th, Prim* prim)
{
	POP(pos, "rot2");
	POP(right, "rot2");
	POP(left, "rot2");
	th.push(makeUGen("Rotate2", rateAR, {left, right, pos}, 2));
}

static void fade2_(Thread& th, Prim* prim)
{
	POP(pos, "fade2");
	POP(right, "fade2");
	POP(left, "fade2");
	th.push(makeUGen("XFade2", rateAR, {left, right, pos, 1.}));
}

////////////////////////////////////////////////////////////////////////////////
// misc

static void sah_(Thread& th, Prim* prim)
{
	POP(trig, "sah");
	POP(in, "sah");
	th.push(makeUGen("Latch", rateAR, {in, trig}));
}

// clip, wrap and fold compute in the interpreter when every input is a real.
static void range(Thread& th, const char* name, const char* ugen, double (*f)(double, double, double))
{
	V hi = popGraphIn(th, name);
	V lo = popGraphIn(th, name);
	V in = popGraphIn(th, name);
	if (in.isReal() && lo.isReal() && hi.isReal()) {
		th.push(f(in.f, lo.f, hi.f));
		return;
	}
	int rate = std::max({rateOf(in), rateOf(lo), rateOf(hi)});
	th.push(makeUGen(ugen, rate, {in, lo, hi}));
}

static double clampf(double x, double lo, double hi) { return std::clamp(x, lo, hi); }
static void clip_(Thread& th, Prim* prim) { range(th, "clip", "Clip", clampf); }
static void wrap_(Thread& th, Prim* prim) { range(th, "wrap", "Wrap", sc_wrap); }
static void fold_(Thread& th, Prim* prim) { range(th, "fold", "Fold", sc_fold); }

// Integer wrap and fold have no SC UGen; they take reals only.
static void irange(Thread& th, const char* name, int64_t (*f)(int64_t, int64_t, int64_t))
{
	int64_t hi = th.popInt(name);
	int64_t lo = th.popInt(name);
	int64_t in = th.popInt(name);
	th.push((double)f(in, lo, hi));
}

static void iwrap_(Thread& th, Prim* prim) { irange(th, "iwrap", sc_iwrap); }
static void ifold_(Thread& th, Prim* prim) { irange(th, "ifold", sc_ifold); }

////////////////////////////////////////////////////////////////////////////////
// envelopes

enum { shapeLinear = 1, shapeExponential = 2 };

// EnvGen over breakpoints levels[0] -> levels[i+1] in times[i].
static V envGen(std::vector<double> levels, std::vector<V> times, int shape, int doneAction)
{
	std::vector<V> in = {1., 1., 0., 1., (double)doneAction, levels[0], (double)times.size(), -99., -99.};
	for (size_t i = 0; i < times.size(); ++i) {
		in.push_back(levels[i + 1]);
		in.push_back(times[i]);
		in.push_back((double)shape);
		in.push_back(0.);
	}
	return makeUGen("EnvGen", rateAR, in);
}

// EnvGen rounds its segment time up to whole samples, so it gets half a sample less.
V endAfter(Thread& th, Arg in, Arg frames)
{
	V seconds = mul(th, graphBinary(th, opSub, frames, .5), makeUGen("SampleDur", rateIR, {}));
	V gate = envGen({1., 1., 0.}, {seconds, 0.}, shapeLinear, 2);
	((Sig*)gate.o())->node->endGate = true;
	return mul(th, in, gate);
}

// Seconds to frames at the server's rate, rounded as sapf rounds them.
V secondsToFrames(Thread& th, double seconds)
{
	return graphUnary(th, opFloor, add(th, mul(th, seconds, makeUGen("SampleRate", rateIR, {})), .5));
}

// sapf fades scale by amp - .001, where amp moves exponentially between .001 and 1.001.
static void fadein_(Thread& th, Prim* prim)
{
	double fade = th.popFloat("fadein : fadeTime");
	POP(in, "fadein");
	V env = envGen({.001, 1., 1.001}, {fade, 0.}, shapeExponential, 0);
	th.push(mul(th, in, graphBinary(th, opSub, env, .001)));
}

// sapf's fadeout ends the signal; the EnvGen frees the synth when the fade completes. sapf's
// first fade sample still has gain 1, so the sustain segment is one sample longer.
static void fadeout_(Thread& th, Prim* prim)
{
	double fade = th.popFloat("fadeout : fadeTime");
	double sustain = th.popFloat("fadeout : sustainTime");
	POP(in, "fadeout");
	V hold = add(th, sustain, makeUGen("SampleDur", rateIR, {}));
	V env = envGen({1.001, 1.001, 1.001 * .001}, {hold, fade}, shapeExponential, 2);
	th.push(mul(th, in, graphBinary(th, opSub, env, .001)));
}

// sapf's line and xline are finite: the synth frees itself after dur.
static void line_(Thread& th, Prim* prim)
{
	double end = th.popFloat("line : end");
	double start = th.popFloat("line : start");
	double dur = th.popFloat("line : dur");
	th.push(endAfter(th, makeUGen("Line", rateAR, {start, end, dur, 0.}), secondsToFrames(th, dur)));
}

// Across zero sapf cubes a linear ramp between cube roots, which XLine cannot do.
static void xline_(Thread& th, Prim* prim)
{
	double end = th.popFloat("xline : end");
	double start = th.popFloat("xline : start");
	double dur = th.popFloat("xline : dur");
	V ramp;
	if (sc_sgn(start) != sc_sgn(end) || start == 0. || end == 0.) {
		V line = makeUGen("Line", rateAR, {sc_sgn(start) * cbrt(fabs(start)), sc_sgn(end) * cbrt(fabs(end)), dur, 0.});
		ramp = graphUnary(th, opCubed, line);
	} else {
		ramp = makeUGen("XLine", rateAR, {start, end, dur, 0.});
	}
	th.push(endAfter(th, ramp, secondsToFrames(th, dur)));
}

////////////////////////////////////////////////////////////////////////////////
// SuperCollider access

static void soundin_(Thread& th, Prim* prim)
{
	POP(chan, "soundin");
	V bus = add(th, makeUGen("NumOutputBuses", rateIR, {}), chan);
	th.push(makeUGen("In", rateAR, {bus}, 1, 0, true, true)); // In.kr reads control buses
}

static void ctl_(Thread& th, Prim* prim)
{
	V name = th.pop();
	if (!name.isString()) wrongType("ctl : name", "String", name);
	double value = th.popFloat("ctl : default");
	th.push(makeParam(((String*)name.o())->s, value));
}

static int parseRate(Arg v)
{
	if (v.isString()) {
		const char* s = ((String*)v.o())->s;
		if (!strcmp(s, "ar")) return rateAR;
		if (!strcmp(s, "kr")) return rateKR;
		if (!strcmp(s, "ir")) return rateIR;
	}
	wrongType("ugen : rate", "'ar, 'kr or 'ir", v);
}

static void ugen_(Thread& th, Prim* prim)
{
	int numOutputs = (int)th.popInt("ugen : numOutputs");
	int rate = parseRate(th.pop());
	V name = th.pop();
	if (!name.isString()) wrongType("ugen : name", "String", name);
	P<List> list = th.popVList("ugen : inputs");
	if (!list->isFinite()) indefiniteOp("ugen : inputs", "");
	list = list->pack(th);
	std::vector<V> inputs;
	for (int64_t i = 0; i < list->mArray->size(); ++i) {
		V in = list->mArray->at(i);
		if (!in.isReal() && !isSig(in)) wrongType("ugen : inputs", "Reals or UGens", in);
		inputs.push_back(in);
	}
	if (numOutputs < 0) throw errOutOfRange;
	th.push(makeUGen(((String*)name.o())->s, rate, inputs, numOutputs, 0, numOutputs > 0, true));
}

static void kr_(Thread& th, Prim* prim)
{
	th.push(toKR(popGraphIn(th, "kr : in")));
}

// Each play sends a new def, so a running synth keeps the def it started with.
static int gPlayCount = 0;

static void play_(Thread& th, Prim* prim)
{
	V channels = th.pop();
	std::string name = "sapf-play-" + std::to_string(++gPlayCount);
	std::string def = compileSynthDef(th, channels, name);
	// /s_new with node ID -1 (let the server choose), add to head of the root group.
	std::string snew = oscMessage("/s_new", {{OscArg::String, 0, name}, {OscArg::Int, -1, ""},
		{OscArg::Int, 0, ""}, {OscArg::Int, 0, ""}});
	std::string err = scsynthSendSync({oscMessage("/d_recv", {{OscArg::Blob, 0, def}, {OscArg::Blob, 0, snew}})});
	if (!err.empty()) {
		post("play : scsynth at %s: %s. Start one with 'make server' or set SAPF_SCSYNTH.\n",
			scsynthAddress().c_str(), err.c_str());
		throw errFailed;
	}
}

static void stop_(Thread& th, Prim* prim)
{
	scsynthSend(oscMessage("/g_deepFree", {{OscArg::Int, 0, ""}}));
}

// The file goes to $SAPF_SYNTHDEFS, or the current directory when it is unset.
static void writesd_(Thread& th, Prim* prim)
{
	V name = th.pop();
	if (!name.isString()) wrongType(">sd : name", "String", name);
	V channels = th.pop();
	std::string defName = ((String*)name.o())->s;
	std::string bytes = compileSynthDef(th, channels, defName);

	const char* dir = getenv("SAPF_SYNTHDEFS");
	std::string path = (dir && *dir) ? std::string(dir) + "/" : std::string();
	path += defName + ".scsyndef";
	std::ofstream f(path, std::ios::binary);
	if (!f || !f.write(bytes.data(), (std::streamsize)bytes.size())) {
		post(">sd : could not write '%s'\n", path.c_str());
		throw errFailed;
	}
	post("wrote '%s'\n", path.c_str());
}

////////////////////////////////////////////////////////////////////////////////

#define DEF(NAME, TAKES, LEAVES, HELP) 	vm.def(#NAME, TAKES, LEAVES, NAME##_, HELP);
#define DEFMCX(NAME, N, HELP) 	vm.defmcx(#NAME, N, NAME##_, HELP);

void AddSCUGenOps();
void AddSCUGenOps()
{
	vm.addBifHelp("\n*** SuperCollider UGens ***");
	DEFMCX(sinosc, 2, "(freq phase --> out) sine wave oscillator. SinOsc.")
	DEFMCX(lfsaw, 2, "(freq phase --> out) non band limited sawtooth wave oscillator. LFSaw.")
	DEFMCX(lftri, 2, "(freq phase --> out) non band limited triangle wave oscillator. LFTri.")
	DEFMCX(lfpulse, 3, "(freq phase duty --> out) non band limited unipolar pulse wave oscillator. LFPulse.")
	DEFMCX(lfpulseb, 3, "(freq phase duty --> out) non band limited bipolar pulse wave oscillator. LFPulse.")
	DEFMCX(lfsquare, 2, "(freq phase --> out) non band limited square wave oscillator. LFPulse.")
	DEFMCX(impulse, 2, "(freq phase --> out) non band limited single sample impulse train oscillator. Impulse.")

	DEFMCX(white, 1, "(amp --> out) white noise. WhiteNoise.")
	DEFMCX(pink, 1, "(amp --> out) pink noise. PinkNoise.")
	DEFMCX(brown, 1, "(amp --> out) brown noise. BrownNoise.")
	DEFMCX(dust, 2, "(density amp --> out) random impulses from 0 to amp. Dust.")
	DEFMCX(dust2, 2, "(density amp --> out) random impulses from -amp to +amp. Dust2.")
	DEFMCX(lfnoise0, 1, "(freq --> out) step noise source. LFNoise0.")
	DEFMCX(lfnoise1, 1, "(freq --> out) ramp noise source. LFNoise1.")

	DEFMCX(lag, 2, "(in decayTime --> out) one pole lag filter. Lag.")
	DEFMCX(lagud, 3, "(in upDecayTime downDecayTime --> out) one pole lag filter. LagUD.")
	DEFMCX(lpf, 2, "(in freq --> out) low pass filter. 12 dB/oct. LPF.")
	DEFMCX(hpf, 2, "(in freq --> out) high pass filter. 12 dB/oct. HPF.")
	DEFMCX(rlpf, 3, "(in freq rq --> out) resonant low pass filter. rq is 1/Q. RLPF.")
	DEFMCX(rhpf, 3, "(in freq rq --> out) resonant high pass filter. rq is 1/Q. RHPF.")
	DEFMCX(bpf, 3, "(in freq bw --> out) band pass filter. bw is bandwidth in octaves. BPF.")
	DEFMCX(bsf, 3, "(in freq bw --> out) band stop filter. bw is bandwidth in octaves. BRF.")
	DEFMCX(resonz, 3, "(in freq rq --> out) resonant filter. Resonz.")
	DEFMCX(ringz, 3, "(in freq ringTime --> out) resonant filter specified by a ring time in seconds. Ringz.")
	DEFMCX(formlet, 4, "(in freq atkTime dcyTime --> out) formant filter. Formlet.")
	DEFMCX(leakdc, 2, "(in coef --> out) leaks away energy at 0 Hz. LeakDC.")
	DEFMCX(decay, 2, "(in decayTime --> out) exponential decay for impulses at the input. Decay.")
	DEFMCX(decay2, 3, "(in atkTime dcyTime --> out) exponential attack and decay for impulses at the input. Decay2.")

	DEFMCX(delayn, 3, "(in delay maxdelay --> out) delay line with no interpolation. DelayN.")
	DEFMCX(delayl, 3, "(in delay maxdelay --> out) delay line with linear interpolation. DelayL.")
	DEFMCX(delayc, 3, "(in delay maxdelay --> out) delay line with cubic interpolation. DelayC.")
	DEFMCX(combn, 4, "(in delay maxdelay decayTime --> out) comb delay filter with no interpolation. CombN.")
	DEFMCX(combl, 4, "(in delay maxdelay decayTime --> out) comb delay filter with linear interpolation. CombL.")
	DEFMCX(combc, 4, "(in delay maxdelay decayTime --> out) comb delay filter with cubic interpolation. CombC.")
	DEFMCX(alpasn, 4, "(in delay maxdelay decayTime --> out) all pass delay filter with no interpolation. AllpassN.")
	DEFMCX(alpasl, 4, "(in delay maxdelay decayTime --> out) all pass delay filter with linear interpolation. AllpassL.")
	DEFMCX(alpasc, 4, "(in delay maxdelay decayTime --> out) all pass delay filter with cubic interpolation. AllpassC.")

	DEFMCX(pan2, 2, "(in pos --> [left right]) stereo pan. pos -1 is full left, +1 full right. Pan2.")
	DEFMCX(bal2, 3, "(left right pos --> [left right]) stereo balance. Balance2.")
	DEFMCX(rot2, 3, "(left right pos --> [left right]) stereo rotation. +/-1 is 180 degrees. Rotate2.")
	DEFMCX(fade2, 3, "(left right pos --> out) equal power cross fade. -1 is all left, +1 all right. XFade2.")

	DEFMCX(sah, 2, "(in trigger --> out) sample and hold. Latch.")
	DEFMCX(clip, 3, "(in lo hi --> out) constrain the input to the bounds by clipping. Clip.")
	DEFMCX(wrap, 3, "(in lo hi --> out) constrain the input to the bounds by wrapping. Wrap.")
	DEFMCX(iwrap, 3, "(in lo hi --> out) constrain the integer input to the bounds by wrapping. reals only.")
	DEFMCX(ifold, 3, "(in lo hi --> out) constrain the integer input to the bounds by folding. reals only.")
	DEFMCX(fold, 3, "(in lo hi --> out) constrain the input to the bounds by folding at the edges. Fold.")

	DEFMCX(line, 3, "(dur start end --> out) linear ramp from start to end in dur seconds, then the synth ends. Line.")
	DEFMCX(xline, 3, "(dur start end --> out) exponential ramp from start to end in dur seconds, then the synth ends. XLine.")
	DEFMCX(fadein, 2, "(in fadeTime --> out) fade in. EnvGen.")
	DEFMCX(fadeout, 3, "(in sustainTime fadeTime --> out) fade out after sustain, then free the synth. EnvGen.")

	DEFMCX(soundin, 1, "(channel --> out) hardware input channel, from 0. In.")
	DEFMCX(kr, 1, "(in --> out) control-rate version of in. UGens feeding only control-rate UGens become control rate too.")
	DEF(ctl, 2, 1, "(default name --> out) control-rate synth parameter. Control.")
	DEF(ugen, 4, 1, "(inputs name rate numOutputs --> out) any SuperCollider UGen. inputs is a list, name a string, rate 'ar 'kr or 'ir.")
	DEF(play, 1, 0, "(channels -->) sends channels as a SynthDef to scsynth at $SAPF_SCSYNTH (default 127.0.0.1:57110) and starts a synth.")
	vm.def("stop", 0, 0, stop_, "(-->) frees every synth on the scsynth that play uses.");
	vm.def(">sd", 2, 0, writesd_, "(channels name -->) writes channels as SynthDef name to name.scsyndef in $SAPF_SYNTHDEFS or the current directory.");
}
