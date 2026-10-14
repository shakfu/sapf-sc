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
#include "Texture.hpp"
#include "Samples.hpp"
#include <chrono>
#include "MathFuns.hpp"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>

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

// SC's SinOscFB has no phase input and reads its inputs once per block.
static void sinoscfb_(Thread& th, Prim* prim)
{
	POP(fb, "sinoscfb");
	double phase = th.popFloat("sinoscfb : phase");
	POP(freq, "sinoscfb");
	if (phase != 0.) {
		post("sinoscfb : phase must be 0; SC's SinOscFB has no phase input\n");
		throw errOutOfRange;
	}
	th.push(makeUGen("SinOscFB", rateAR, {freq, fb}));
}

static void sinoscm_(Thread& th, Prim* prim)
{
	POP(add, "sinoscm");
	POP(mul, "sinoscm");
	POP(phase, "sinoscm");
	POP(freq, "sinoscm");
	V osc = makeUGen("SinOsc", rateAR, {freq, cyclesToRadians(th, phase)});
	th.push(::add(th, ::mul(th, osc, mul), add));
}

// SC's LFSaw, LFTri and LFPulse take an initial phase: LFSaw's in half cycles, LFTri's in quarter
// cycles, LFPulse's in cycles. sapf's phase is in cycles.
static double popPhase(Thread& th, const char* name)
{
	return th.popFloat((std::string(name) + " : phase").c_str());
}

static V popFreq(Thread& th, const char* name)
{
	return popGraphIn(th, (std::string(name) + " : freq").c_str());
}

static void sawWave(Thread& th, const char* name)
{
	double phase = popPhase(th, name);
	V freq = popFreq(th, name);
	th.push(makeUGen("LFSaw", rateAR, {freq, sc_wrap(phase * 2., 0., 2.)}));
}

// LFTri's phase runs from -1 to 3 and starts the rising segment at -1.
static void triWave(Thread& th, const char* name, double quarters, double span)
{
	double phase = popPhase(th, name);
	V freq = popFreq(th, name);
	th.push(makeUGen("LFTri", rateAR, {freq, sc_wrap(phase * quarters, -1., span - 1.)}));
}

static void squareWave(Thread& th, const char* name)
{
	double phase = popPhase(th, name);
	V freq = popFreq(th, name);
	th.push(unibi(th, makeUGen("LFPulse", rateAR, {freq, sc_wrap(phase, 0., 1.), .5})));
}

static void lfsaw_(Thread& th, Prim* prim) { sawWave(th, "lfsaw"); }
// sapf's lftri phase moves along the rising segment only: .5 starts at -1.
static void lftri_(Thread& th, Prim* prim) { triWave(th, "lftri", 2., 2.); }
static void lfsquare_(Thread& th, Prim* prim) { squareWave(th, "lfsquare"); }

// saw, tri, square and pulse are sapf's band-limited oscillators. Original sapf's tables for them
// are broken (docs/dev/upstream-sapf-bugs.md), so these give the waveforms the tables describe,
// without band limiting.
static void saw_(Thread& th, Prim* prim) { sawWave(th, "saw"); }
static void tri_(Thread& th, Prim* prim) { triWave(th, "tri", 4., 4.); }
static void square_(Thread& th, Prim* prim) { squareWave(th, "square"); }

// sapf's pulse is half the difference of two saws duty apart: 1 - duty for a duty fraction of each
// cycle, ending at half cycle, and -duty elsewhere. LFPulse reads its width once per cycle.
static void pulse_(Thread& th, Prim* prim)
{
	POP(duty, "pulse");
	double phase = popPhase(th, "pulse");
	POP(freq, "pulse");
	V ramp = makeUGen("LFSaw", rateAR, {freq, sc_wrap(phase * 2., -1., 1.)});
	V high = graphBinary(th, opGE, ramp, graphBinary(th, opSub, 1., mul(th, duty, 2.)));
	th.push(graphBinary(th, opSub, high, duty));
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

// Both interpolate a Catmull-Rom spline through random points; sapf's points are within .8.
static void lfnoise3_(Thread& th, Prim* prim)
{
	POP(freq, "lfnoise3");
	th.push(mul(th, makeUGen("LFDNoise3", rateAR, {freq}), .8));
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

// One input or a list of them per filter; a list sets the filter count, its shortest length.
static std::vector<V> perFilter(Thread& th, Arg v, const char* msg)
{
	if (!v.isVList()) {
		if (!v.isReal() && !isSig(v)) wrongType(msg, "Real, UGen or list of them", v);
		return {v};
	}
	if (!v.isFinite()) indefiniteOp(msg, "");
	P<List> list = ((List*)v.o())->pack(th);
	std::vector<V> out;
	for (int64_t i = 0; i < list->mArray->size(); ++i) {
		V x = list->mArray->at(i);
		if (!x.isReal() && !isSig(x)) wrongType(msg, "Real or UGen", x);
		out.push_back(x);
	}
	return out;
}

// sapf's klank is a sum of its ringz filters, each times its amp.
static void klank_(Thread& th, Prim* prim)
{
	std::vector<V> times = perFilter(th, th.pop(), "klank : ringTimes");
	std::vector<V> amps = perFilter(th, th.pop(), "klank : amps");
	std::vector<V> freqs = perFilter(th, th.pop(), "klank : freqs");
	POP(in, "klank");
	auto at = [](const std::vector<V>& v, size_t i) { return v.size() == 1 ? v[0] : v[i]; };
	size_t n = SIZE_MAX;
	for (auto* v : {&freqs, &amps, &times})
		if (v->size() != 1) n = std::min(n, v->size());
	if (n == SIZE_MAX) n = 1;
	V out;
	for (size_t i = 0; i < n; ++i) {
		V ring = mul(th, makeUGen("Ringz", rateAR, {in, at(freqs, i), at(times, i)}), at(amps, i));
		out = i == 0 ? ring : add(th, out, ring);
	}
	th.push(out);
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

// sapf's first-order filters: a1 = b - sqrt(b^2 - 1) with b = 2 - cos(w). FOS takes the coefficients.
static V firstOrderCoeff(Thread& th, Arg freq)
{
	V w = graphBinary(th, opFDiv, mul(th, freq, kTwoPi), makeUGen("SampleRate", rateIR, {}));
	V b = graphBinary(th, opSub, 2., graphUnary(th, opCos, w));
	return graphBinary(th, opSub, b, graphUnary(th, opSqrt, graphBinary(th, opSub, mul(th, b, b), 1.)));
}

static void lpf1_(Thread& th, Prim* prim)
{
	POP(freq, "lpf1");
	POP(in, "lpf1");
	V a1 = firstOrderCoeff(th, freq);
	V scale = mul(th, graphBinary(th, opSub, 1., a1), .5);
	th.push(makeUGen("FOS", rateAR, {in, scale, scale, a1}));
}

static void hpf1_(Thread& th, Prim* prim)
{
	POP(freq, "hpf1");
	POP(in, "hpf1");
	V a1 = firstOrderCoeff(th, freq);
	V scale = mul(th, add(th, a1, 1.), .5);
	th.push(makeUGen("FOS", rateAR, {in, scale, graphUnary(th, opNeg, scale), a1}));
}

// An RBJ biquad section: SOS with b and a divided by a0, feedback negated.
struct Biquad {
	V b0, b1, b2, a1, a2, a0;

	V apply(Thread& th, Arg in) const
	{
		auto norm = [&](Arg x) { return graphBinary(th, opFDiv, x, a0); };
		return makeUGen("SOS", rateAR, {in, norm(b0), norm(b1), norm(b2), norm(graphUnary(th, opNeg, a1)),
			norm(graphUnary(th, opNeg, a2))});
	}
};

// sapf's 24 dB/oct filters run the same biquad twice. alpha is sin(w) times alphaScale.
static Biquad rbj(Thread& th, Arg freq, Arg alphaScale, bool high)
{
	V w = graphBinary(th, opFDiv, mul(th, freq, kTwoPi), makeUGen("SampleRate", rateIR, {}));
	V cs = graphUnary(th, opCos, w);
	V alpha = mul(th, graphUnary(th, opSin, w), alphaScale);
	V b1 = high ? graphBinary(th, opSub, -1., cs) : graphBinary(th, opSub, 1., cs);
	V b0 = mul(th, b1, high ? -.5 : .5);
	return {b0, b1, b0, mul(th, cs, -2.), graphBinary(th, opSub, 1., alpha), add(th, alpha, 1.)};
}

static void pass2(Thread& th, const char* name, bool high)
{
	V freq = popGraphIn(th, (std::string(name) + " : freq").c_str());
	V in = popGraphIn(th, (std::string(name) + " : in").c_str());
	Biquad f = rbj(th, graphBinary(th, opMax, freq, 1e-3), .5 * M_SQRT2, high);
	th.push(f.apply(th, f.apply(th, in)));
}

static void lpf2_(Thread& th, Prim* prim) { pass2(th, "lpf2", false); }
static void hpf2_(Thread& th, Prim* prim) { pass2(th, "hpf2", true); }

static void rlpf2_(Thread& th, Prim* prim)
{
	POP(rq, "rlpf2");
	POP(freq, "rlpf2");
	POP(in, "rlpf2");
	Biquad f = rbj(th, freq, mul(th, rq, .5), false);
	th.push(f.apply(th, f.apply(th, in)));
}

static void lag2_(Thread& th, Prim* prim)
{
	POP(time, "lag2");
	POP(in, "lag2");
	th.push(makeUGen("Lag2", rateAR, {in, time}));
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

// sapf's flange: in delayed by delay * SR + half frames, at least 2, minus in delayed by half
// frames, half = ceil(SR * maxdelay + .5). flangep adds them. sapf interpolates Lagrange, DelayC Hermite.
static void flanger(Thread& th, const char* name, int op)
{
	std::string n(name);
	double maxdelay = th.popFloat((n + " : maxdelay").c_str());
	V delay = popGraphIn(th, (n + " : delay").c_str());
	V in = popGraphIn(th, (n + " : in").c_str());
	maxdelay = maxDelay(name, maxdelay, delay);
	V sr = makeUGen("SampleRate", rateIR, {});
	V half = graphUnary(th, opCeil, add(th, mul(th, sr, maxdelay), .5));
	V bufTime = graphBinary(th, opFDiv, add(th, mul(th, half, 2.), 4.), sr);
	V clamped = graphBinary(th, opMax, graphBinary(th, opMin, delay, maxdelay), -maxdelay);
	V frames = graphBinary(th, opMax, add(th, mul(th, clamped, sr), half), 2.);
	V wet = makeUGen("DelayC", rateAR, {in, bufTime, graphBinary(th, opFDiv, frames, sr)});
	V dry = makeUGen("DelayN", rateAR, {in, bufTime, graphBinary(th, opFDiv, half, sr)});
	th.push(graphBinary(th, op, wet, dry));
}

static void flange_(Thread& th, Prim* prim) { flanger(th, "flange", opSub); }
static void flangep_(Thread& th, Prim* prim) { flanger(th, "flangep", opAdd); }

////////////////////////////////////////////////////////////////////////////////
// panning

// sapf's itd delays left by (pan + 1) * half frames and right by (1 - pan) * half, at least 2,
// with half = ceil(SR * maxdelay / 2 + .5). sapf interpolates Lagrange, DelayC Hermite.
static void itd_(Thread& th, Prim* prim)
{
	double maxdelay = th.popFloat("itd : maxdelay");
	POP(pan, "itd");
	POP(in, "itd");
	V sr = makeUGen("SampleRate", rateIR, {});
	V half = graphUnary(th, opCeil, add(th, mul(th, sr, maxdelay * .5), .5));
	V bufTime = graphBinary(th, opFDiv, add(th, mul(th, half, 2.), 3.), sr);
	auto delay = [&](Arg p) {
		V frames = graphBinary(th, opMax, mul(th, add(th, p, 1.), half), 2.);
		return makeUGen("DelayC", rateAR, {in, bufTime, graphBinary(th, opFDiv, frames, sr)});
	};
	P<List> out = new List(itemTypeV, 2);
	out->add(delay(pan));
	out->add(delay(graphUnary(th, opNeg, pan)));
	th.push(out);
}

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

static void madd_(Thread& th, Prim* prim)
{
	POP(c, "*+");
	POP(b, "*+");
	POP(a, "*+");
	th.push(add(th, mul(th, a, b), c));
}

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
static V envGen(std::vector<double> levels, std::vector<V> times, int shape, int doneAction, V gate = 1.,
	double releaseNode = -99.)
{
	std::vector<V> in = {gate, 1., 0., 1., (double)doneAction, levels[0], (double)times.size(), releaseNode, -99.};
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

// sapf's adsr: attack and decay in seconds, sustain until noteDur beats at the tempo, then release
// from the level reached, as an EnvGen whose gate closes at note off. The curves are sapf's.
static void adsr_(Thread& th, Prim* prim)
{
	double rate = th.popFloat("adsr : tempo");
	double noteDur = th.popFloat("adsr : noteDur");
	double amp = th.popFloat("adsr : amp");
	P<List> list = th.popList("adsr : [attack decay sustain release]");
	Z env[4];
	if (list->fillz(th, 4, env) != 4) {
		post("adsr : [attack decay sustain release] list should have 4 elements.\n");
		throw errFailed;
	}
	if (rate <= 0.) {
		post("adsr : tempo must be positive\n");
		throw errOutOfRange;
	}
	double atk = env[0], dcy = env[1], sus = env[2], rel = env[3];
	V beats = makeUGen("Sweep", rateAR, {0., rate});
	V gate = graphBinary(th, opLT, beats, noteDur);
	const double curve = 5.;
	V eg = makeUGen("EnvGen", rateAR, {gate, 1., 0., 1., 0., 0., 3., 2., -99.,
		amp, atk, curve, -1., amp * sus, dcy, curve, -5., 0., rel, curve, -5.});
	V seconds = graphUnary(th, opCeil, mul(th, noteDur / rate + rel, makeUGen("SampleRate", rateIR, {})));
	th.push(endAfter(th, eg, seconds));
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
// sequencing

// A Dseq holds the values in the SynthDef. An infinite stream keeps its first kMaxItems values,
// looped over its period when it repeats within them.
enum { kMaxItems = 1024 };

// Rows of values pulled from sapf streams, one value from each stream per row.
struct Items {
	size_t width;
	std::vector<double> v;
	bool finite;

	size_t rows() const { return v.size() / width; }
	double at(size_t row, size_t col) const { return v[row * width + col]; }

	// Rows a Dseq needs: all of a finite stream, else one period.
	size_t length() const
	{
		size_t n = rows();
		if (finite) return n;
		for (size_t p = 1; p <= n / 2; ++p)
			if (std::equal(v.begin() + p * width, v.end(), v.begin())) return p;
		return n;
	}
};

// Pulls rows until a stream ends or kMaxItems rows are kept. A real repeats forever. `keep`
// drops rows that sapf skips. `lead` takes one value from the first stream before the rows.
static Items readItems(Thread& th, std::vector<V> streams, const char* msg, bool (*keep)(const double*) = nullptr,
	double* lead = nullptr)
{
	Items it = {streams.size(), {}, false};
	std::vector<BothIn> in(streams.size());
	for (size_t j = 0; j < streams.size(); ++j) {
		Arg s = streams[j];
		if (s.isList() && s.isFinite()) {
			it.finite = true;
			if (((List*)s.o())->length(th) > kMaxItems) {
				post("%s : more than %d values\n", msg, kMaxItems);
				throw errOutOfRange;
			}
		} else if (!s.isReal() && !s.isList()) {
			wrongType(msg, "Real or list of reals", s);
		}
		in[j].set(s);
	}
	V first;
	if (lead && !in[0].one(th, first) && first.isReal()) {
		*lead = first.f;
	} else if (lead) {
		post("%s : no values\n", msg);
		throw errFailed;
	}
	std::vector<double> row(streams.size());
	for (size_t pulls = 0; it.rows() < kMaxItems && pulls < 64 * kMaxItems; ++pulls) {
		for (size_t j = 0; j < streams.size(); ++j) {
			V v;
			if (in[j].one(th, v)) {
				it.finite = true;
				goto done;
			}
			if (!v.isReal()) wrongType(msg, "Real", v);
			row[j] = v.f;
		}
		if (!keep || keep(row.data())) it.v.insert(it.v.end(), row.begin(), row.end());
	}
done:
	if (it.rows() == 0) {
		post("%s : no values\n", msg);
		throw errFailed;
	}
	return it;
}

static V dseq(const Items& it, size_t col, double scale = 1.)
{
	size_t n = it.length();
	std::vector<V> in = {it.finite ? 1. : INFINITY};
	for (size_t i = 0; i < n; ++i) in.push_back(it.at(i, col) * scale);
	return makeUGen("Dseq", rateDR, in, 1, 0, false);
}

static V popTrigger(Thread& th, const char* msg)
{
	V trig = popGraphIn(th, msg);
	if (rateOf(trig) < rateKR) wrongType(msg, "audio or control rate UGen", trig);
	return trig;
}

// Demand reads its source on each trigger. A finite source ends the signal on the trigger after
// its last value.
static V demand(Thread& th, Arg in, Arg trig, const char* msg)
{
	Items it = readItems(th, {in}, msg);
	V out = makeUGen("Demand", rateOf(trig), {trig, 0., dseq(it, 0)}, 1, 0, false);
	if (!it.finite) return out;
	// The EnvGen frees the synth one sample late, so the comparison zeroes the ending sample.
	V count = makeUGen("PulseCount", rateOf(trig), {trig, 0.});
	V more = graphBinary(th, opLE, count, (double)it.rows());
	V gate = envGen({1., 0.}, {0.}, shapeLinear, 2, more, 0.);
	((Sig*)gate.o())->node->endGate = true;
	return mul(th, mul(th, out, more), gate);
}

static void seq_(Thread& th, Prim* prim)
{
	V trig = popTrigger(th, "seq : trigger");
	V in = th.pop();
	th.push(demand(th, in, trig, "seq : in"));
}

static void iseq_(Thread& th, Prim* prim)
{
	V trig = popTrigger(th, "iseq : trigger");
	V in = th.pop();
	V out = demand(th, in, trig, "iseq : in");
	th.push(mul(th, out, graphBinary(th, opGT, trig, 0.)));
}

static bool positiveDur(const double* row) { return row[1] > 0.; }

// steps and imps: Duty and TDuty over (value, duration) pairs; sapf skips pairs with a duration
// at or below zero. sapf integrates the rate within a step and Duty cannot, so rate is a real.
static void duty(Thread& th, const char* name, const char* ugen, std::vector<V> extra)
{
	std::string msg = std::string(name) + " : values and durs";
	double rate = th.popFloat(name);
	V durs = th.pop();
	V vals = th.pop();
	if (rate <= 0.) {
		post("%s : rate must be positive\n", name);
		throw errOutOfRange;
	}
	Items it = readItems(th, {vals, durs}, msg.c_str(), positiveDur);
	std::vector<V> in = {dseq(it, 1, 1. / rate), 0., 0., dseq(it, 0)};
	in.insert(in.end(), extra.begin(), extra.end());
	V out = makeUGen(ugen, rateAR, in, 1, 0, false);
	if (it.finite) {
		double total = 0.;
		for (size_t i = 0; i < it.rows(); ++i) total += it.at(i, 1);
		out = endAfter(th, out, graphUnary(th, opCeil, mul(th, total / rate, makeUGen("SampleRate", rateIR, {}))));
	}
	th.push(out);
}

static void steps_(Thread& th, Prim* prim) { duty(th, "steps", "Duty", {}); }
static void imps_(Thread& th, Prim* prim) { duty(th, "imps", "TDuty", {0.}); }

// lines, xlines and curves: DemandEnvGen from the first value through each later value in its
// duration, with a curvature per segment for curves. sapf jumps to the value of a segment with
// duration zero or less; DemandEnvGen reaches it in one frame and keeps later segments on time.
static void envLines(Thread& th, const char* name, int shape, bool curved)
{
	std::string msg = std::string(name) + " : values and durs";
	double rate = th.popFloat(name);
	V durs = th.pop();
	V curves = curved ? th.pop() : V(0.);
	V vals = th.pop();
	if (rate <= 0.) {
		post("%s : rate must be positive\n", name);
		throw errOutOfRange;
	}
	std::vector<V> streams = {vals, durs};
	if (curved) streams.push_back(curves);
	double first;
	Items it = readItems(th, streams, msg.c_str(), nullptr, &first);
	size_t n = it.length();
	double total = 0.;
	for (size_t i = 0; i < it.rows(); ++i) total += std::max(it.at(i, 1), 0.);
	// DemandEnvGen's constructor advances one frame, and each segment outputs its first step
	// rather than its start, so a two-frame segment holding the first value comes first. An
	// infinite stream's period loops in a nested Dseq.
	auto seq = [&](V lead, V head, size_t col) {
		std::vector<V> in = {1., lead};
		if (!head.isReal() || !std::isnan(head.f)) in.push_back(head);
		if (it.finite)
			for (size_t i = 0; i < n; ++i) in.push_back(it.at(i, col));
		else
			in.push_back(dseq(it, col));
		return makeUGen("Dseq", rateDR, in, 1, 0, false);
	};
	V frame = mul(th, makeUGen("SampleDur", rateIR, {}), 2. * rate);
	V level = seq(first, first, 0);
	V dur = seq(frame, NAN, 1);
	V curve = curved ? seq(0., NAN, 2) : V(0.);
	V out = makeUGen("DemandEnvGen", rateAR, {level, dur, (double)shape, curve, 1., 0., 1., 0., 1. / rate, 0.}, 1, 0,
		false);
	if (it.finite)
		out = endAfter(th, out, graphUnary(th, opCeil, mul(th, total / rate, makeUGen("SampleRate", rateIR, {}))));
	th.push(out);
}

static void lines_(Thread& th, Prim* prim) { envLines(th, "lines", 1, false); }
static void xlines_(Thread& th, Prim* prim) { envLines(th, "xlines", 2, false); }
static void curves_(Thread& th, Prim* prim) { envLines(th, "curves", 5, true); }

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
	// at the tail of the root group, after any ola voices it reads; its ola voices stop when it
	// ends, so it has a node ID of its own, above the 1000.. sclang uses
	const int node = 2000000 + gPlayCount;
	std::string snew = oscMessage("/s_new", {{OscArg::String, 0, name}, {OscArg::Int, node, ""},
		{OscArg::Int, 1, ""}, {OscArg::Int, 0, ""}});
	auto loaded = std::make_shared<LoadedTables>();
	std::vector<std::string> packets = loadWavetables(th, channels, *loaded);
	for (std::string const& m : loadSamples(th, channels, *loaded, true)) packets.push_back(m);
	packets.push_back(oscMessage("/d_recv", {{OscArg::Blob, 0, def}, {OscArg::Blob, 0, snew}}));
	std::string err = scsynthSendSync(packets);
	if (!err.empty()) {
		post("play : scsynth at %s: %s. Start one with 'make server' or set SAPF_SCSYNTH.\n",
			scsynthAddress().c_str(), err.c_str());
		throw errFailed;
	}
	double now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
	playTextures(th, channels, now + .2, loaded, node);
}

static void stop_(Thread& th, Prim* prim)
{
	stopTextures();
	scsynthSend(oscMessage("/clearSched", {})); // ola voices already sent for later
	scsynthSend(oscMessage("/g_deepFree", {{OscArg::Int, 0, ""}}));
}

// Files go to $SAPF_SYNTHDEFS, or the current directory when it is unset.
static void writeFile(const char* word, const std::string& name, const std::string& bytes)
{
	const char* dir = getenv("SAPF_SYNTHDEFS");
	std::string path = (dir && *dir) ? std::string(dir) + "/" : std::string();
	path += name;
	std::ofstream f(path, std::ios::binary);
	if (!f || !f.write(bytes.data(), (std::streamsize)bytes.size())) {
		post("%s : could not write '%s'\n", word, path.c_str());
		throw errFailed;
	}
	post("wrote '%s'\n", path.c_str());
}

static std::string popName(Thread& th, const char* msg)
{
	V name = th.pop();
	if (!name.isString()) wrongType(msg, "String", name);
	return ((String*)name.o())->s;
}

static void writesd_(Thread& th, Prim* prim)
{
	std::string name = popName(th, ">sd : name");
	V channels = th.pop();
	if (!texturesOf(th, channels).empty()) {
		post(">sd : a SynthDef cannot start ola voices; use play or >nrt\n");
		throw errFailed;
	}
	if (!wavetableNodesOf(th, channels).empty()) {
		post(">sd : osc reads scsynth buffers, which a SynthDef cannot load; use play or >nrt\n");
		throw errFailed;
	}
	writeFile(">sd", name + ".scsyndef", compileSynthDef(th, channels, name));
	// scsy loads the sound files its sidecar names; other lines of an existing sidecar stay.
	const char* dir = getenv("SAPF_SYNTHDEFS");
	std::string sidecarPath = ((dir && *dir) ? std::string(dir) + "/" : std::string()) + name + ".txt";
	std::ifstream in(sidecarPath, std::ios::binary);
	std::string old((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
	std::string sidecar = sidecarWithSamples(th, channels, old);
	bool named = !sampleNodesOf(th, channels).empty() || old.find("@buf") != std::string::npos;
	if (named && sidecar != old) writeFile(">sd", name + ".txt", sidecar);
}

// sapf's >sf: name.wav in $SAPF_RECORDINGS, else the temp folder, at the interpreter's rate, as
// long as the first channel to end. Rendered by the embedded scsynth in NRT mode.
static std::string renderFile(Thread& th, const char* word)
{
	std::string name = popName(th, (std::string(word) + " : filename").c_str());
	V channels = th.pop();
	double sr = th.rate.sampleRate;
	double seconds = renderSeconds(th, channels, sr);
	int64_t frames = (int64_t)std::llround(seconds * sr);
	const char* dir = getenv("SAPF_RECORDINGS");
	std::error_code ec;
	std::string path = ((dir && *dir) ? std::string(dir) : std::filesystem::temp_directory_path(ec).string()) +
		"/" + name + ".wav";
	int n = channelCount(th, channels);
	std::string error = scsynthRender(nrtScore(th, channels, "sapf-render", seconds), path, n, frames, sr);
	if (!error.empty()) {
		post("%s : %s\n", word, error.c_str());
		throw errFailed;
	}
	post("wrote file '%s'  %d channels  %g secs\n", path.c_str(), n, seconds);
	return path;
}

static void sfwrite_(Thread& th, Prim* prim) { renderFile(th, ">sf"); }

static void sfwriteopen_(Thread& th, Prim* prim)
{
	std::string path = renderFile(th, ">sfo");
#if defined(__APPLE__)
	std::string cmd = "open \"" + path + "\"";
#else
	std::string cmd = "xdg-open \"" + path + "\" &";
#endif
	if (system(cmd.c_str()) != 0) post(">sfo : could not open '%s'\n", path.c_str());
}

static void writenrt_(Thread& th, Prim* prim)
{
	std::string name = popName(th, ">nrt : name");
	double seconds = th.popFloat(">nrt : seconds");
	V channels = th.pop();
	writeFile(">nrt", name + ".osc", nrtScore(th, channels, name, seconds));
}

////////////////////////////////////////////////////////////////////////////////

#define DEF(NAME, TAKES, LEAVES, HELP) 	vm.def(#NAME, TAKES, LEAVES, NAME##_, HELP);
#define DEFMCX(NAME, N, HELP) 	vm.defmcx(#NAME, N, NAME##_, HELP);
#define DEFAM(NAME, MASK, HELP) 	vm.defautomap(#NAME, #MASK, NAME##_, HELP);

void AddSCUGenOps();
void AddSCUGenOps()
{
	vm.addBifHelp("\n*** SuperCollider UGens ***");
	DEFMCX(sinosc, 2, "(freq phase --> out) sine wave oscillator. SinOsc.")
	DEFMCX(sinoscfb, 3, "(freq phase feedback --> out) sine wave oscillator with self feedback phase modulation. phase must be 0. SinOscFB.")
	DEFMCX(sinoscm, 4, "(freq phase mul add --> out) sine wave oscillator with multiply and add. SinOsc.")
	DEFMCX(lfsaw, 2, "(freq phase --> out) non band limited sawtooth wave oscillator. LFSaw.")
	DEFMCX(lftri, 2, "(freq phase --> out) non band limited triangle wave oscillator. LFTri.")
	DEFMCX(lfpulse, 3, "(freq phase duty --> out) non band limited unipolar pulse wave oscillator. LFPulse.")
	DEFMCX(lfpulseb, 3, "(freq phase duty --> out) non band limited bipolar pulse wave oscillator. LFPulse.")
	DEFMCX(lfsquare, 2, "(freq phase --> out) non band limited square wave oscillator. LFPulse.")
	DEFMCX(saw, 2, "(freq phase --> out) sawtooth wave oscillator, not band limited. LFSaw.")
	DEFMCX(tri, 2, "(freq phase --> out) triangle wave oscillator, not band limited. LFTri.")
	DEFMCX(square, 2, "(freq phase --> out) square wave oscillator, not band limited. LFPulse.")
	DEFMCX(pulse, 3, "(freq phase duty --> out) pulse wave oscillator from -duty to 1 - duty, not band limited. LFSaw.")
	DEFMCX(impulse, 2, "(freq phase --> out) non band limited single sample impulse train oscillator. Impulse.")

	DEFMCX(white, 1, "(amp --> out) white noise. WhiteNoise.")
	DEFMCX(pink, 1, "(amp --> out) pink noise. PinkNoise.")
	DEFMCX(brown, 1, "(amp --> out) brown noise. BrownNoise.")
	DEFMCX(dust, 2, "(density amp --> out) random impulses from 0 to amp. Dust.")
	DEFMCX(dust2, 2, "(density amp --> out) random impulses from -amp to +amp. Dust2.")
	DEFMCX(lfnoise0, 1, "(freq --> out) step noise source. LFNoise0.")
	DEFMCX(lfnoise1, 1, "(freq --> out) ramp noise source. LFNoise1.")

	DEFMCX(lfnoise3, 1, "(freq --> out) cubic spline noise source. LFDNoise3.")

	DEFMCX(lag, 2, "(in decayTime --> out) one pole lag filter. Lag.")
	DEFMCX(lag2, 2, "(in decayTime --> out) two one pole lag filters in series. Lag2.")
	DEFMCX(lagud, 3, "(in upDecayTime downDecayTime --> out) one pole lag filter. LagUD.")
	DEFMCX(lpf, 2, "(in freq --> out) low pass filter. 12 dB/oct. LPF.")
	DEFMCX(hpf, 2, "(in freq --> out) high pass filter. 12 dB/oct. HPF.")
	DEFMCX(lpf1, 2, "(in freq --> out) low pass filter. 6 dB/oct. FOS.")
	DEFMCX(hpf1, 2, "(in freq --> out) high pass filter. 6 dB/oct. FOS.")
	DEFMCX(lpf2, 2, "(in freq --> out) low pass filter. 24 dB/oct. SOS.")
	DEFMCX(hpf2, 2, "(in freq --> out) high pass filter. 24 dB/oct. SOS.")
	DEFMCX(rlpf2, 3, "(in freq rq --> out) resonant low pass filter. 24 dB/oct. rq is 1/Q. SOS.")
	DEFMCX(rlpf, 3, "(in freq rq --> out) resonant low pass filter. rq is 1/Q. RLPF.")
	DEFMCX(rhpf, 3, "(in freq rq --> out) resonant high pass filter. rq is 1/Q. RHPF.")
	DEFMCX(bpf, 3, "(in freq bw --> out) band pass filter. bw is bandwidth in octaves. BPF.")
	DEFMCX(bsf, 3, "(in freq bw --> out) band stop filter. bw is bandwidth in octaves. BRF.")
	DEFMCX(resonz, 3, "(in freq rq --> out) resonant filter. Resonz.")
	DEFMCX(ringz, 3, "(in freq ringTime --> out) resonant filter specified by a ring time in seconds. Ringz.")
	DEFMCX(formlet, 4, "(in freq atkTime dcyTime --> out) formant filter. Formlet.")
	DEFAM(klank, zaaa, "(in freqs amps ringTimes --> out) a bank of ringz filters. freqs amps and ringTimes are lists. Ringz.")
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

	DEFAM(itd, zzk, "(in pan maxdelay --> [left right]) interaural time delay. DelayC.")
	DEFAM(flange, zzk, "(in delay maxdelay --> out) flanger with cubic interpolation. delay can be negative. latency is maxdelay. DelayC.")
	DEFAM(flangep, zzk, "(in delay maxdelay --> out) flanger with cubic interpolation. adds delayed signal instead of subtracts. DelayC.")
	DEFMCX(pan2, 2, "(in pos --> [left right]) stereo pan. pos -1 is full left, +1 full right. Pan2.")
	DEFMCX(bal2, 3, "(left right pos --> [left right]) stereo balance. Balance2.")
	DEFMCX(rot2, 3, "(left right pos --> [left right]) stereo rotation. +/-1 is 180 degrees. Rotate2.")
	DEFMCX(fade2, 3, "(left right pos --> out) equal power cross fade. -1 is all left, +1 all right. XFade2.")

	DEFMCX(sah, 2, "(in trigger --> out) sample and hold. Latch.")
	DEFMCX(clip, 3, "(in lo hi --> out) constrain the input to the bounds by clipping. Clip.")
	DEFMCX(wrap, 3, "(in lo hi --> out) constrain the input to the bounds by wrapping. Wrap.")
	vm.defmcx("*+", 3, madd_, "(a b c --> out) multiply add. a b * c +");
	DEFMCX(iwrap, 3, "(in lo hi --> out) constrain the integer input to the bounds by wrapping. reals only.")
	DEFMCX(ifold, 3, "(in lo hi --> out) constrain the integer input to the bounds by folding. reals only.")
	DEFMCX(fold, 3, "(in lo hi --> out) constrain the input to the bounds by folding at the edges. Fold.")

	DEFMCX(line, 3, "(dur start end --> out) linear ramp from start to end in dur seconds, then the synth ends. Line.")
	DEFMCX(xline, 3, "(dur start end --> out) exponential ramp from start to end in dur seconds, then the synth ends. XLine.")
	DEFAM(adsr, akkz, "([attack decay sustain release] amp dur tempo --> envelope) an envelope generator. tempo must be a real. EnvGen.")
	DEFMCX(fadein, 2, "(in fadeTime --> out) fade in. EnvGen.")
	DEFMCX(fadeout, 3, "(in sustainTime fadeTime --> out) fade out after sustain, then free the synth. EnvGen.")

	DEFAM(seq, az, "(in trigger --> out) pulls one value from the input for each trigger. output sustains at that level until the next trigger. Demand.")
	DEFAM(iseq, az, "(in trigger --> out) pulls one value from the input for each trigger. outputs that value for one sample. outputs zero when there is no trigger. Demand.")
	DEFAM(steps, aaz, "(values durs rate --> out) each value for its duration. Duty.")
	DEFAM(imps, aaz, "(values durs rate --> out) each value as a single sample impulse at the start of its duration. TDuty.")

	DEFAM(lines, aaz, "(values durs rate --> out) linear segments between values. DemandEnvGen.")
	DEFAM(xlines, aaz, "(values durs rate --> out) exponential segments between values. DemandEnvGen.")
	DEFAM(curves, aaaz, "(values curvatures durs rate --> out) curved segments between values. DemandEnvGen.")

	DEFMCX(soundin, 1, "(channel --> out) hardware input channel, from 0. In.")
	DEFMCX(kr, 1, "(in --> out) control-rate version of in. UGens feeding only control-rate UGens become control rate too.")
	DEF(ctl, 2, 1, "(default name --> out) control-rate synth parameter. Control.")
	DEF(ugen, 4, 1, "(inputs name rate numOutputs --> out) any SuperCollider UGen. inputs is a list, name a string, rate 'ar 'kr or 'ir.")
	DEF(play, 1, 0, "(channels -->) sends channels as a SynthDef to scsynth at $SAPF_SCSYNTH (default 127.0.0.1:57110) and starts a synth, and any ola voices it reads.")
	vm.def("stop", 0, 0, stop_, "(-->) frees every synth on the scsynth that play uses.");
	vm.def(">sf", 2, 0, sfwrite_, "(channels filename -->) renders the channels to filename.wav in $SAPF_RECORDINGS or the temp folder, until the first channel ends. scsynth NRT.");
	vm.def(">sfo", 2, 0, sfwriteopen_, "(channels filename -->) as >sf, then opens the file in the default application.");
	vm.def(">nrt", 3, 0, writenrt_, "(channels seconds name -->) writes an scsynth NRT score of channels and their ola voices to name.osc in $SAPF_SYNTHDEFS or the current directory.");
	vm.def(">sd", 2, 0, writesd_, "(channels name -->) writes channels as SynthDef name to name.scsyndef in $SAPF_SYNTHDEFS or the current directory.");
}
