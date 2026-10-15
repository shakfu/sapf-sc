//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Graph.hpp"
#include "MathFuns.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <map>
#include <unordered_map>

static std::atomic<int64_t> gNodeSerial{0};

UGenNode::UGenNode(const char* inName, int inRate, std::vector<V> inInputs, int inNumOutputs, int inSpecial, bool inPure)
	: name(inName), rate(inRate), special(inSpecial), numOutputs(inNumOutputs), pure(inPure),
	  inputs(std::move(inInputs)), serial(gNodeSerial++)
{
}

static const char* rateName(int rate)
{
	switch (rate) {
		case rateIR : return "ir";
		case rateKR : return "kr";
		case rateDR : return "dr";
		default : return "ar";
	}
}

void Sig::print(Thread& th, std::string& out, int depth)
{
	char s[300];
	if (node->isParam)
		snprintf(s, sizeof(s), "%s.kr", node->name.c_str());
	else if (node->numOutputs > 1)
		snprintf(s, sizeof(s), "%s.%s[%d]", node->name.c_str(), rateName(node->rate), index);
	else
		snprintf(s, sizeof(s), "%s.%s", node->name.c_str(), rateName(node->rate));
	out += s;
}

V popGraphIn(Thread& th, const char* msg)
{
	V v = th.pop();
	if (v.isRef() || v.isZRef()) v = v.deref();
	if (!v.isReal() && !isSig(v))
		wrongType(msg, "Real or UGen", v);
	return v;
}

V outputsOf(P<UGenNode> const& node)
{
	int numOutputs = node->numOutputs;
	if (numOutputs == 1)
		return new Sig(node, 0);
	P<List> list = new List(itemTypeV, numOutputs);
	for (int i = 0; i < numOutputs; ++i)
		list->add(V(new Sig(node, i)));
	return list;
}

V makeUGen(const char* name, int rate, std::vector<V> inputs, int numOutputs, int special, bool pure, bool fixedRate)
{
	P<UGenNode> node = new UGenNode(name, rate, std::move(inputs), numOutputs, special, pure);
	node->fixedRate = fixedRate;
	return outputsOf(node);
}

V toKR(Arg v)
{
	if (!isSig(v)) return v;
	Sig* s = (Sig*)v.o();
	UGenNode* u = s->node();
	if (u->rate != rateAR) return v;
	if (u->fixedRate) return makeUGen("A2K", rateKR, {v});
	P<UGenNode> copy = new UGenNode(u->name.c_str(), rateKR, u->inputs, u->numOutputs, u->special, u->pure);
	copy->krOf = u;
	copy->spec = u->spec;
	return new Sig(copy, s->index);
}

V toIR(Arg v)
{
	if (!isSig(v)) return v;
	Sig* s = (Sig*)v.o();
	UGenNode* u = s->node->krOf ? s->node->krOf() : s->node();
	if (u->rate == rateIR) return new Sig(u, s->index);
	P<UGenNode> copy = new UGenNode(u->name.c_str(), rateIR, u->inputs, u->numOutputs, u->special, u->pure);
	copy->krOf = u;
	copy->spec = u->spec;
	return new Sig(copy, s->index);
}

V makeParam(const char* name, double value, int kind, int order)
{
	static const int rates[] = {rateKR, rateIR, rateKR, rateAR};
	P<UGenNode> node = new UGenNode(name, rates[kind], {}, 1, 0, true);
	node->isParam = true;
	node->paramDefault = value;
	node->paramKind = kind;
	node->paramOrder = order;
	return new Sig(node, 0);
}

std::vector<P<UGenNode>>* gDefRoots = nullptr;

////////////////////////////////////////////////////////////////////////////////
// operators

static bool foldBinary(int special, double a, double b, double& out)
{
	switch (special) {
		case opAdd : out = a + b; return true;
		case opSub : out = a - b; return true;
		case opMul : out = a * b; return true;
		case opFDiv : out = a / b; return true;
		default : return false;
	}
}

V graphBinary(Thread& th, int special, Arg a, Arg b)
{
	double z;
	if (a.isReal() && b.isReal() && foldBinary(special, a.f, b.f, z))
		return z;
	int rate = std::max(rateOf(a), rateOf(b));
	return makeUGen("BinaryOpUGen", rate, {a, b}, 1, special);
}

V graphUnary(Thread& th, int special, Arg a)
{
	if (a.isReal() && special == opNeg)
		return -a.f;
	return makeUGen("UnaryOpUGen", rateOf(a), {a}, 1, special);
}

// sapf unary op name -> SC special index.
static const std::unordered_map<std::string, int> gUnaryMap = {
	{"neg", opNeg}, {"abs", opAbs}, {"ceil", opCeil}, {"floor", opFloor}, {"frac", opFrac},
	{"sgn", opSign}, {"sq", opSquared}, {"cb", opCubed}, {"sqrt", opSqrt}, {"exp", opExp},
	{"recip", opRecip}, {"nnhz", opMIDICPS}, {"hznn", opCPSMIDI}, {"semiratio", opMIDIRatio},
	{"ratiosemi", opRatioMIDI}, {"dbamp", opDbAmp}, {"ampdb", opAmpDb}, {"log", opLog},
	{"log2", opLog2}, {"log10", opLog10}, {"sin", opSin}, {"cos", opCos}, {"tan", opTan},
	{"asin", opArcSin}, {"acos", opArcCos}, {"atan", opArcTan}, {"sinh", opSinH}, {"cosh", opCosH},
	{"tanh", opTanH}, {"distort", opDistort}, {"softclip", opSoftClip},
};

// sapf unary ops with no single SC operator, built from several.
static V compositeUnary(Thread& th, const std::string& name, Arg a)
{
	auto bin = [&](int sp, Arg x, Arg y) { return graphBinary(th, sp, x, y); };
	auto un = [&](int sp, Arg x) { return graphUnary(th, sp, x); };
	if (name == "ohz") return un(opOctCPS, bin(opAdd, a, 4.));
	if (name == "hzo") return bin(opSub, un(opCPSOct, a), 4.);
	if (name == "centsratio") return un(opMIDIRatio, bin(opMul, a, .01));
	if (name == "ratiocents") return bin(opMul, un(opRatioMIDI, a), 100.);
	if (name == "half") return bin(opMul, a, .5);
	if (name == "twice") return bin(opMul, a, 2.);
	if (name == "inc") return bin(opAdd, a, 1.);
	if (name == "dec") return bin(opSub, a, 1.);
	if (name == "cmpl") return bin(opSub, 1., a);
	if (name == "biuni") return bin(opAdd, bin(opMul, a, .5), .5);
	if (name == "unibi") return bin(opSub, bin(opMul, a, 2.), 1.);
	if (name == "sin1") return un(opSin, bin(opMul, a, kTwoPi));
	if (name == "cos1") return un(opCos, bin(opMul, a, kTwoPi));
	if (name == "degrad") return bin(opMul, a, kDegToRad);
	if (name == "raddeg") return bin(opMul, a, kRadToDeg);
	if (name == "rsqrt") return un(opRecip, un(opSqrt, a));
	if (name == "exp2") return bin(opPow, 2., a);
	if (name == "exp10") return bin(opPow, 10., a);
	if (name == "sigm") return bin(opFDiv, a, un(opSqrt, bin(opAdd, 1., un(opSquared, a))));
	if (name == "not" || name == "iszero") return bin(opEQ, a, 0.);
	if (name == "isneg") return bin(opLT, a, 0.);
	if (name == "ispos") return bin(opGT, a, 0.);
	if (name == "nonneg") return bin(opGE, a, 0.);
	if (name == "nonpos") return bin(opLE, a, 0.);
	return V();
}

V Sig::unaryOp(Thread& th, UnaryOp* op)
{
	std::string name = op->Name();
	if (name == "ToZero") return 0.; // sapf's list * 0 maps this over the list
	auto it = gUnaryMap.find(name);
	if (it != gUnaryMap.end())
		return graphUnary(th, it->second, this);
	V r = compositeUnary(th, name, this);
	if (r.isObject())
		return r;
	post("%s : no SuperCollider equivalent for UGen input.\n", name.c_str());
	throw errUndefinedOperation;
}

// sapf binary op name -> SC special index.
static const std::unordered_map<std::string, int> gBinaryMap = {
	{"plus", opAdd}, {"plus_link", opAdd}, {"minus", opSub}, {"mul", opMul}, {"div", opFDiv}, {"idiv", opIDiv},
	{"mod", opMod}, {"eq", opEQ}, {"ne", opNE}, {"lt", opLT}, {"gt", opGT}, {"le", opLE},
	{"ge", opGE}, {"min", opMin}, {"max", opMax}, {"lcm", opLCM}, {"gcd", opGCD},
	{"round", opRound}, {"roundUp", opRoundUp}, {"trunc", opTrunc}, {"atan2", opAtan2},
	{"hypot", opHypot}, {"pow", opPow}, {"ring1", opRing1}, {"ring2", opRing2}, {"ring3", opRing3},
	{"ring4", opRing4}, {"difsq", opDifSqr}, {"sumsq", opSumSqr}, {"sqsum", opSqrSum},
	{"sqdif", opSqrDif}, {"absdif", opAbsDif}, {"thresh", opThresh}, {"amclip", opAMClip},
	{"scaleneg", opScaleNeg}, {"clip2", opClip2}, {"excess", opExcess}, {"fold2", opFold2},
	{"wrap2", opWrap2},
};

static V sigBinary(Thread& th, BinaryOp* op, Arg a, Arg b)
{
	std::string name = op->Name();
	auto it = gBinaryMap.find(name);
	if (it != gBinaryMap.end())
		return graphBinary(th, it->second, a, b);
	if (name == "avg2")
		return graphBinary(th, opMul, graphBinary(th, opAdd, a, b), .5);
	post("%s : no SuperCollider equivalent for UGen input.\n", name.c_str());
	throw errUndefinedOperation;
}

V Sig::binaryOp(Thread& th, BinaryOp* op, Arg _b)
{
	V b = _b;
	if (b.isRef()) b = b.deref();
	if (b.isVList())
		return op->makeVList(th, this, b);
	if (b.isReal() || isSig(b))
		return sigBinary(th, op, this, b);
	wrongType(op->Name(), "Real, List or UGen", b);
}

V Sig::binaryOpWithReal(Thread& th, BinaryOp* op, Z _a)
{
	return sigBinary(th, op, _a, this);
}

V Sig::binaryOpWithVList(Thread& th, BinaryOp* op, List* _a)
{
	return op->makeVList(th, _a, this);
}

V Sig::binaryOpWithZList(Thread& th, BinaryOp* op, List* _a)
{
	post("%s : a signal (ZList) cannot be combined with a UGen.\n", op->Name());
	throw errWrongType;
}

////////////////////////////////////////////////////////////////////////////////
// SCgf compiler

void flattenChannels(Thread& th, Arg v, std::vector<V>& out)
{
	if (v.isReal() || isSig(v)) {
		out.push_back(v);
	} else if (v.isVList()) {
		if (!v.isFinite()) indefiniteOp(">sd : channels", "");
		P<List> list = ((List*)v.o())->pack(th);
		Array* a = list->mArray();
		for (int64_t i = 0; i < a->size(); ++i)
			flattenChannels(th, a->at(i), out);
	} else {
		wrongType(">sd : channels", "UGen, Real or List of them", v);
	}
}

namespace {

struct Writer {
	std::string bytes;
	void u8(int x) { bytes += (char)(uint8_t)x; }
	void u16(int x) { u8(x >> 8); u8(x); }
	void u32(uint32_t x) { u8(x >> 24); u8(x >> 16); u8(x >> 8); u8(x); }
	void f32(double x)
	{
		float f = (float)x;
		uint32_t u;
		memcpy(&u, &f, 4);
		u32(u);
	}
	void str(const std::string& s)
	{
		if (s.size() > 255) {
			post("SCgf string exceeds 255 bytes: %s\n", s.c_str());
			throw errOutOfRange;
		}
		for (unsigned char c : s) {
			if (c > 127) {
				post("SCgf strings must be ASCII: %s\n", s.c_str());
				throw errOutOfRange;
			}
		}
		u8((int)s.size());
		bytes += s;
	}
};

struct SortBundle {
	std::vector<UGenNode*> antecedents;
	std::vector<UGenNode*> descendants;
};

template <class T>
void appendUnique(std::vector<T>& v, T x)
{
	if (std::find(v.begin(), v.end(), x) == v.end()) v.push_back(x);
}

// Port of nanosynth's SynthDefBuilder._sort_topologically, so that sapf and nanosynth emit
// the same UGen order for the same creation order.
std::vector<UGenNode*> sortTopologically(const std::vector<UGenNode*>& ugens)
{
	std::map<UGenNode*, SortBundle> bundles;
	std::map<UGenNode*, size_t> position;
	for (size_t i = 0; i < ugens.size(); ++i) position[ugens[i]] = i;
	for (UGenNode* u : ugens) bundles[u];
	for (UGenNode* u : ugens) {
		for (Arg in : u->inputs) {
			if (!isSig(in)) continue;
			UGenNode* src = ((Sig*)in.o())->node();
			appendUnique(bundles[u].antecedents, src);
			appendUnique(bundles[src].descendants, u);
		}
	}
	for (UGenNode* u : ugens) {
		auto& d = bundles[u].descendants;
		std::sort(d.begin(), d.end(), [&](UGenNode* x, UGenNode* y) { return position[x] < position[y]; });
	}

	std::vector<UGenNode*> available;
	std::vector<UGenNode*> output;
	for (auto it = ugens.rbegin(); it != ugens.rend(); ++it) {
		if (bundles[*it].antecedents.empty())
			appendUnique(available, *it);
	}
	while (!available.empty()) {
		UGenNode* u = available.back();
		available.pop_back();
		auto& desc = bundles[u].descendants;
		for (auto it = desc.rbegin(); it != desc.rend(); ++it) {
			auto& ante = bundles[*it].antecedents;
			ante.erase(std::find(ante.begin(), ante.end(), u));
			if (ante.empty())
				appendUnique(available, *it);
		}
		output.push_back(u);
	}
	if (output.size() != ugens.size()) {
		post(">sd : cycle in UGen graph\n");
		throw errFailed;
	}
	return output;
}

void collect(UGenNode* u, std::vector<UGenNode*>& out, std::map<UGenNode*, bool>& seen)
{
	if (seen[u]) return;
	seen[u] = true;
	out.push_back(u);
	for (Arg in : u->inputs)
		if (isSig(in)) collect(((Sig*)in.o())->node(), out, seen);
}

// The graph compiled for one def. Graph values are immutable and may go into other defs, so the
// compiler works on copies of the nodes reachable from the channels.
struct WorkGraph {
	std::map<UGenNode*, bool> used;               // originals referenced directly
	std::map<UGenNode*, P<UGenNode>> work;        // original -> copy
	std::map<UGenNode*, P<UGenNode>> krWork;      // original -> copy of its kr version
	std::map<UGenNode*, P<UGenNode>> irWork;      // original -> copy of its ir version
	std::vector<P<UGenNode>> riders;              // riders of the nodes reached, once each
	std::map<std::pair<UGenNode*, int>, V> a2k;   // (original, output) -> A2K of its copy
	std::vector<P<UGenNode>> keep;                // nodes made during compilation, alive until it ends

	void markUsed(Arg v)
	{
		if (!isSig(v)) return;
		UGenNode* u = ((Sig*)v.o())->node();
		for (UGenNode* owner : {u, u->krOf()})
			if (owner)
				for (P<UGenNode> const& r : owner->riders)
					if (std::find(riders.begin(), riders.end(), r) == riders.end()) {
						riders.push_back(r);
						for (Arg in : r->inputs) markUsed(in);
					}
		if (u->krOf) {
			for (Arg in : u->inputs) markUsed(in);
			return;
		}
		if (used[u]) return;
		used[u] = true;
		for (Arg in : u->inputs) markUsed(in);
	}

	UGenNode* copyOf(UGenNode* u, int rate, std::map<UGenNode*, P<UGenNode>>& memo)
	{
		auto it = memo.find(u);
		if (it != memo.end()) return it->second();
		P<UGenNode> c = new UGenNode(u->name.c_str(), rate, {}, u->numOutputs, u->special, u->pure);
		c->serial = u->serial;
		c->isParam = u->isParam;
		c->paramDefault = u->paramDefault;
		c->paramKind = u->paramKind;
		c->paramOrder = u->paramOrder;
		c->fixedRate = u->fixedRate;
		c->endGate = u->endGate;
		c->texture = u->texture;
		c->wavetable = u->wavetable;
		c->tableBuf = u->tableBuf;
		c->sample = u->sample;
		c->spec = u->spec;
		memo[u] = c;
		// A generated UGen's kr method takes its inputs as given; its ar method raised some.
		auto& from = rate != rateAR && !u->krInputs.empty() ? u->krInputs : u->inputs;
		for (Arg in : from) c->inputs.push_back(remap(in));
		return c();
	}

	// kr copies of one node merge into one. When the node is also used at audio rate, its kr
	// uses read it through A2K, so a random UGen stays one source.
	V remap(Arg v)
	{
		if (!isSig(v)) return v;
		Sig* s = (Sig*)v.o();
		UGenNode* u = s->node();
		if (!u->krOf)
			return new Sig(copyOf(u, u->rate, work), s->index);
		UGenNode* orig = u->krOf();
		// ir has no conversion from audio rate, so an ir copy is its own node
		if (u->rate == rateIR)
			return new Sig(copyOf(orig, rateIR, irWork), s->index);
		if (!used[orig])
			return new Sig(copyOf(orig, rateKR, krWork), s->index);
		auto key = std::make_pair(orig, s->index);
		if (!a2k.count(key))
			a2k[key] = makeUGen("A2K", rateKR, {V(new Sig(copyOf(orig, orig->rate, work), s->index))});
		return a2k[key];
	}
};

// In sapf, anything computed from a finite signal ends with it. A channel that depends on end
// gates is multiplied by them, so it is zero from the frame its signal ends, not from the end
// of the block in which the synth frees.
V endChannel(Thread& th, Arg chan)
{
	if (!isSig(chan)) return chan;
	std::vector<UGenNode*> nodes;
	std::map<UGenNode*, bool> seen;
	collect(((Sig*)chan.o())->node(), nodes, seen);
	V out = chan;
	for (UGenNode* u : nodes) {
		if (!u->endGate || u == ((Sig*)chan.o())->node()) continue;
		out = graphBinary(th, opMul, out, V(new Sig(u, 0)));
	}
	return out;
}

// Lowers audio-rate nodes to control rate where every consumer is a control-rate sapf UGen. Out,
// which consumes the channels, is audio rate. So is a ugen node: a control-rate analysis UGen
// such as Amplitude reads its whole audio input, not one sample per block. Generated UGen words
// take no part (docs/dev/stac.md, open question 1).
void inferRates(const std::vector<V>& chans)
{
	std::vector<UGenNode*> nodes;
	std::map<UGenNode*, bool> seen;
	std::map<UGenNode*, std::vector<UGenNode*>> consumers;
	std::map<UGenNode*, bool> feedsOut;
	for (Arg c : chans) {
		if (!isSig(c)) continue;
		feedsOut[((Sig*)c.o())->node()] = true;
		collect(((Sig*)c.o())->node(), nodes, seen);
	}
	for (UGenNode* u : nodes)
		for (Arg in : u->inputs)
			if (isSig(in)) consumers[((Sig*)in.o())->node()].push_back(u);
	for (bool changed = true; changed;) {
		changed = false;
		for (UGenNode* u : nodes) {
			if (u->rate != rateAR || u->fixedRate || u->spec >= 0 || feedsOut[u] || consumers[u].empty()) continue;
			auto& cs = consumers[u];
			// A generated UGen word (spec >= 0) runs at the rate written, and may read its whole audio
			// input at kr, as FFT does; it neither lowers nor is lowered.
			if (std::all_of(cs.begin(), cs.end(),
					[](UGenNode* c) { return c->rate == rateKR && !c->fixedRate && c->spec < 0; })) {
				u->rate = rateKR;
				changed = true;
			}
		}
	}
}

} // namespace

// `v` as a number at sample rate `sr`, when it is a constant expression of SampleRate and SampleDur.
static bool evalConstant(Arg v, double sr, double& out)
{
	if (v.isReal()) {
		out = v.f;
		return true;
	}
	if (!isSig(v)) return false;
	UGenNode* u = ((Sig*)v.o())->node();
	if (u->name == "SampleRate") return out = sr, true;
	if (u->name == "SampleDur") return out = 1. / sr, true;
	double a, b;
	if (u->name == "UnaryOpUGen" && evalConstant(u->inputs[0], sr, a)) {
		switch (u->special) {
			case opNeg : return out = -a, true;
			case opFloor : return out = floor(a), true;
			case opCeil : return out = ceil(a), true;
			default : return false;
		}
	}
	if (u->name == "BinaryOpUGen" && evalConstant(u->inputs[0], sr, a) && evalConstant(u->inputs[1], sr, b)) {
		switch (u->special) {
			case opMin : return out = std::min(a, b), true;
			case opMax : return out = std::max(a, b), true;
			default : return foldBinary(u->special, a, b, out);
		}
	}
	return false;
}

// When an EnvGen frees the synth or ends a signal, in frames: its segment times, summed and
// scaled, rounded up as EnvGen rounds them. INFINITY when it never does; NAN when the time
// depends on a signal or a gate.
static double envEndFrames(UGenNode* u, double sr)
{
	const auto& in = u->inputs;
	double doneAction, releaseNode, timeScale, gate;
	if (!evalConstant(in[4], sr, doneAction) || (doneAction != 2. && !u->endGate)) return INFINITY;
	if (!evalConstant(in[0], sr, gate) || !evalConstant(in[3], sr, timeScale) ||
		!evalConstant(in[7], sr, releaseNode) || releaseNode != -99.)
		return NAN;
	double seconds = 0.;
	for (size_t i = 10; i < in.size(); i += 4) {
		double t;
		if (!evalConstant(in[i], sr, t)) return NAN;
		seconds += t;
	}
	return ceil(seconds * timeScale * sr - 1e-6);
}

int channelCount(Thread& th, Arg channels)
{
	std::vector<V> chans;
	flattenChannels(th, channels, chans);
	return (int)chans.size();
}

double renderSeconds(Thread& th, Arg channels, double sr)
{
	std::vector<V> chans;
	flattenChannels(th, channels, chans);
	double frames = INFINITY;
	for (Arg c : chans) {
		if (!isSig(c)) continue;
		std::vector<UGenNode*> nodes;
		std::map<UGenNode*, bool> seen;
		collect(((Sig*)c.o())->node(), nodes, seen);
		for (UGenNode* u : nodes) {
			if (u->name != "EnvGen") continue;
			double end = envEndFrames(u, sr);
			if (std::isnan(end)) {
				post(">sf : a channel ends when a signal decides, so its length is unknown; take a part of it with N or T\n");
				throw errFailed;
			}
			frames = std::min(frames, end);
		}
	}
	if (std::isinf(frames)) indefiniteOp(">sf : channels - all have an indefinite number of frames; take a part with N or T", "");
	return frames / sr;
}

std::vector<P<Object>> texturesOf(Thread& th, Arg channels)
{
	std::vector<V> chans;
	flattenChannels(th, channels, chans);
	std::vector<UGenNode*> nodes;
	std::map<UGenNode*, bool> seen;
	for (Arg c : chans)
		if (isSig(c)) collect(((Sig*)c.o())->node(), nodes, seen);
	std::vector<P<Object>> out;
	for (UGenNode* u : nodes)
		if (u->texture()) out.push_back(u->texture);
	return out;
}

static std::vector<UGenNode*> nodesOf(Thread& th, Arg channels, P<Object> UGenNode::*field)
{
	std::vector<V> chans;
	flattenChannels(th, channels, chans);
	std::vector<UGenNode*> nodes;
	std::map<UGenNode*, bool> seen;
	for (Arg c : chans)
		if (isSig(c)) collect(((Sig*)c.o())->node(), nodes, seen);
	std::vector<UGenNode*> out;
	for (UGenNode* u : nodes)
		if ((u->*field)()) out.push_back(u);
	return out;
}

std::vector<UGenNode*> wavetableNodesOf(Thread& th, Arg channels) { return nodesOf(th, channels, &UGenNode::wavetable); }
std::vector<UGenNode*> sampleNodesOf(Thread& th, Arg channels) { return nodesOf(th, channels, &UGenNode::sample); }

std::string compileSynthDef(Thread& th, Arg channels, const std::string& name, int outBus)
{
	DefParts parts;
	parts.outBus = (double)outBus;
	return compileDef(th, channels, name, parts);
}

std::string compileDef(Thread& th, Arg channels, const std::string& name, DefParts const& parts)
{
	std::vector<V> original;
	flattenChannels(th, channels, original);
	if (original.empty() && parts.roots.empty()) {
		post(">sd : no channels\n");
		throw errFailed;
	}
	WorkGraph g;
	for (Arg c : original) g.markUsed(c);
	for (Arg p : parts.params) g.markUsed(p);
	g.markUsed(parts.outBus);
	for (auto& r : parts.roots)
		for (Arg in : r->inputs) g.markUsed(in);
	std::vector<V> chans;
	for (Arg c : original) chans.push_back(endChannel(th, g.remap(c)));
	std::vector<UGenNode*> roots;
	for (auto& r : parts.roots) roots.push_back(g.copyOf(r(), r->rate, g.work));
	for (auto& r : g.riders) roots.push_back(g.copyOf(r(), r->rate, g.work));
	std::vector<V> inferFrom = chans;
	for (UGenNode* r : roots) inferFrom.push_back(V(new Sig(r, 0)));
	inferRates(inferFrom);

	std::vector<UGenNode*> ugens;
	std::map<UGenNode*, bool> seen;
	P<UGenNode> outNode;
	if (!chans.empty()) {
		// Out.ar takes audio-rate inputs only.
		std::vector<V> outInputs = {g.remap(parts.outBus)};
		for (Arg c : chans) {
			if (c.isReal())
				outInputs.push_back(makeUGen("DC", rateAR, {c}));
			else if (rateOf(c) != rateAR)
				outInputs.push_back(makeUGen("K2A", rateAR, {c}));
			else
				outInputs.push_back(c);
		}
		outNode = new UGenNode("Out", rateAR, outInputs, 0, 0, false);
		collect(outNode(), ugens, seen);
	}
	for (UGenNode* r : roots) collect(r, ugens, seen);
	for (Arg p : parts.params) {
		V c = g.remap(p);
		if (isSig(c)) collect(((Sig*)c.o())->node(), ugens, seen);
	}
	std::sort(ugens.begin(), ugens.end(), [](UGenNode* a, UGenNode* b) { return a->serial < b->serial; });

	// Parameters become outputs of Control UGens, one per kind in SC's order: ir, tr, ar, kr. A def's
	// parameters keep their declared order within a kind; ctl's follow, sorted by name.
	std::vector<UGenNode*> params;
	std::vector<UGenNode*> rest;
	for (UGenNode* u : ugens) (u->isParam ? params : rest).push_back(u);
	static const int kindRank[] = {3, 0, 1, 2}; // by paramKR, paramIR, paramTR, paramAR
	std::stable_sort(params.begin(), params.end(), [](UGenNode* a, UGenNode* b) {
		if (kindRank[a->paramKind] != kindRank[b->paramKind]) return kindRank[a->paramKind] < kindRank[b->paramKind];
		bool ad = a->paramOrder >= 0, bd = b->paramOrder >= 0;
		if (ad != bd) return ad;
		if (ad) return a->paramOrder < b->paramOrder;
		return a->name < b->name;
	});
	std::map<std::string, int> seenNames;
	for (UGenNode* p : params) {
		if (seenNames[p->name]++) {
			post(">sd : parameter '%s' is defined more than once\n", p->name.c_str());
			throw errFailed;
		}
	}
	std::vector<P<UGenNode>> controls;
	std::map<UGenNode*, std::pair<UGenNode*, int>> paramOut;
	for (size_t i = 0; i < params.size();) {
		size_t j = i;
		while (j < params.size() && params[j]->paramKind == params[i]->paramKind) ++j;
		static const char* names[] = {"Control", "Control", "TrigControl", "AudioControl"};
		static const int rates[] = {rateKR, rateIR, rateKR, rateAR};
		int kind = params[i]->paramKind;
		P<UGenNode> control = new UGenNode(names[kind], rates[kind], {}, (int)(j - i), (int)i, true);
		for (size_t k = i; k < j; ++k) paramOut[params[k]] = {control(), (int)(k - i)};
		controls.push_back(control);
		i = j;
	}
	for (UGenNode* u : rest)
		for (V& in : u->inputs)
			if (isSig(in) && ((Sig*)in.o())->node->isParam) {
				auto& [control, index] = paramOut[((Sig*)in.o())->node()];
				in = new Sig(control, index);
			}
	for (size_t k = controls.size(); k-- > 0;) rest.insert(rest.begin(), controls[k]());
	g.keep.insert(g.keep.end(), controls.begin(), controls.end()); // alive with the other copies
	if (outNode) g.keep.push_back(outNode);

	std::vector<UGenNode*> ordered = sortTopologically(rest);

	std::map<UGenNode*, int> ugenIndex;
	for (size_t i = 0; i < ordered.size(); ++i) ugenIndex[ordered[i]] = (int)i;
	// Constants dedupe by value, as in nanosynth, with all NaNs equal.
	std::vector<double> constants;
	auto constantIndex = [&](double x) {
		auto it = std::find_if(constants.begin(), constants.end(),
			[x](double c) { return c == x || (std::isnan(c) && std::isnan(x)); });
		return it - constants.begin();
	};
	for (UGenNode* u : ordered)
		for (Arg in : u->inputs)
			if (in.isReal() && constantIndex(in.f) == (ptrdiff_t)constants.size())
				constants.push_back(in.f);

	Writer w;
	w.bytes += "SCgf";
	w.u32(2);
	w.u16(1);
	w.str(name);
	w.u32((uint32_t)constants.size());
	for (double c : constants) w.f32(c);
	w.u32((uint32_t)params.size());
	for (UGenNode* p : params) w.f32(p->paramDefault);
	w.u32((uint32_t)params.size());
	for (size_t i = 0; i < params.size(); ++i) {
		w.str(params[i]->name);
		w.u32((uint32_t)i);
	}
	w.u32((uint32_t)ordered.size());
	for (UGenNode* u : ordered) {
		w.str(u->name);
		w.u8(u->rate);
		w.u32((uint32_t)u->inputs.size());
		w.u32((uint32_t)u->numOutputs);
		w.u16(u->special);
		for (Arg in : u->inputs) {
			if (isSig(in)) {
				Sig* s = (Sig*)in.o();
				w.u32((uint32_t)ugenIndex[s->node()]);
				w.u32((uint32_t)s->index);
			} else {
				w.u32(0xFFFFFFFF);
				w.u32((uint32_t)constantIndex(in.f));
			}
		}
		for (int i = 0; i < u->numOutputs; ++i) w.u8(u->rate);
	}
	w.u16(0); // no variants
	return w.bytes;
}
