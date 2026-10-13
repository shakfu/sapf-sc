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

V makeUGen(const char* name, int rate, std::vector<V> inputs, int numOutputs, int special, bool pure)
{
	P<UGenNode> node = new UGenNode(name, rate, std::move(inputs), numOutputs, special, pure);
	if (numOutputs == 1)
		return new Sig(node, 0);
	P<List> list = new List(itemTypeV, numOutputs);
	for (int i = 0; i < numOutputs; ++i)
		list->add(V(new Sig(node, i)));
	return list;
}

V makeParam(const char* name, double value)
{
	P<UGenNode> node = new UGenNode(name, rateKR, {}, 1, 0, true);
	node->isParam = true;
	node->paramDefault = value;
	return new Sig(node, 0);
}

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

namespace {

// Appends `v` to `out` if it is a Sig or real; recurses into finite lists.
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

} // namespace

std::string compileSynthDef(Thread& th, Arg channels, const std::string& name)
{
	std::vector<V> chans;
	flattenChannels(th, channels, chans);
	if (chans.empty()) {
		post(">sd : no channels\n");
		throw errFailed;
	}

	// Out.ar takes audio-rate inputs only.
	std::vector<V> outInputs = {0.};
	for (Arg c : chans) {
		if (c.isReal())
			outInputs.push_back(makeUGen("DC", rateAR, {c}));
		else if (rateOf(c) != rateAR)
			outInputs.push_back(makeUGen("K2A", rateAR, {c}));
		else
			outInputs.push_back(c);
	}
	P<UGenNode> outNode = new UGenNode("Out", rateAR, outInputs, 0, 0, false);

	std::vector<UGenNode*> ugens;
	std::map<UGenNode*, bool> seen;
	collect(outNode(), ugens, seen);
	std::sort(ugens.begin(), ugens.end(), [](UGenNode* a, UGenNode* b) { return a->serial < b->serial; });

	// Parameters become outputs of one Control, sorted by name, placed first.
	std::vector<UGenNode*> params;
	std::vector<UGenNode*> rest;
	for (UGenNode* u : ugens) (u->isParam ? params : rest).push_back(u);
	std::stable_sort(params.begin(), params.end(), [](UGenNode* a, UGenNode* b) { return a->name < b->name; });
	for (size_t i = 1; i < params.size(); ++i) {
		if (params[i]->name == params[i-1]->name) {
			post(">sd : parameter '%s' is defined more than once\n", params[i]->name.c_str());
			throw errFailed;
		}
	}
	P<UGenNode> control;
	std::map<UGenNode*, int> paramIndex;
	if (!params.empty()) {
		control = new UGenNode("Control", rateKR, {}, (int)params.size(), 0, true);
		for (size_t i = 0; i < params.size(); ++i) paramIndex[params[i]] = (int)i;
		rest.insert(rest.begin(), control());
	}

	// Graph values are immutable and may go into other defs, so the compiler works on copies,
	// with parameters remapped to the Control.
	std::vector<P<UGenNode>> copies;
	std::map<UGenNode*, UGenNode*> copyOf;
	for (UGenNode* u : rest) {
		copies.push_back(new UGenNode(u->name.c_str(), u->rate, {}, u->numOutputs, u->special, u->pure));
		copyOf[u] = copies.back()();
	}
	for (UGenNode* u : rest) {
		for (Arg in : u->inputs) {
			if (!isSig(in)) {
				copyOf[u]->inputs.push_back(in);
				continue;
			}
			Sig* s = (Sig*)in.o();
			if (s->node->isParam)
				copyOf[u]->inputs.push_back(V(new Sig(copyOf[control()], paramIndex[s->node()])));
			else
				copyOf[u]->inputs.push_back(V(new Sig(copyOf[s->node()], s->index)));
		}
	}
	std::vector<UGenNode*> ordered;
	for (UGenNode* u : rest) ordered.push_back(copyOf[u]);
	ordered = sortTopologically(ordered);

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
