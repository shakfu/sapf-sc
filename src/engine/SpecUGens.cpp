//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// stac's generated UGen words (docs/dev/stac.md, phase 2). Each word takes SC's arguments in SC's
// order and units, positionally or as a keyword Form, and builds the class named in its row of
// SpecUGens.inc. Words live in the namespace sc; `import sc.*` makes them global.

#include "Graph.hpp"
#include "Env.hpp"
#include "Namespace.hpp"
#include "Resources.hpp"
#include "symbol.hpp"
#include <cmath>
#include <map>
#include <string>

namespace {

enum WireKind : uint8_t { wireArg, wireSplice, wireSize, wireSpliceTo, wireFlop, wireSpliceWrap, wireEnv, wireEnvInterp };
enum OutputsKind : uint8_t { outputsFixed, outputsSize, outputsValue };

struct SpecArg {
	const char* name;
	double def;
	bool required;
	int fallback;     // when omitted, takes this argument's value, as sclang's `resetval ? min`; or -1
};

struct SpecWire {
	WireKind kind;
	uint8_t arg;
	uint8_t sizeArg; // wireSpliceTo: the argument whose list size the splice takes; wireSpliceWrap: whose value
};

struct SpecUGen {
	const char* ns;
	const char* word;
	const char* cls;
	const char* help;
	int rates;        // bit per rate the class offers
	int rate;         // the highest of them; calls build at this rate
	int numArgs;
	int positional;   // arguments a positional call takes; the rest take defaults
	const SpecArg* args;
	int numWire;
	const SpecWire* wire;
	int outputs;
	OutputsKind outputsKind;
	int outputsArg;   // the argument whose list size (outputsSize) or value (outputsValue) is the count
	int special;
	bool pure;
	int doneInput;    // wire input of doneAction, or -1
	unsigned audio;   // bit per argument the ar method raises to audio rate, as asAudioRateInput does
	int numFill;      // wireFlop: rows after the first that may be omitted,
	const double* fill; // and the value each takes when it is
};

#include "SpecUGens.inc"

const int kNumSpecUGens = (int)(sizeof(kSpecUGens) / sizeof(kSpecUGens[0]));


// NS.word, or the plain word for a bare UGen bound globally.
std::string wordName(const SpecUGen& u)
{
	return std::string("sc.") + u.word;
}

std::string argName(const SpecUGen& u, int i)
{
	return wordName(u) + " : " + u.args[i].name;
}

// A UGen argument: a real, a Sig, an Env, or a finite list of them, packed. build() rejects an Env
// outside an envelope input.
V checkArg(Thread& th, const SpecUGen& u, int i, V v)
{
	if (v.isRef() || v.isZRef()) v = v.deref();
	if (ServerResource* r = asResource(v)) return (double)r->mIndex; // a Bus, Buffer or Node: its index
	if (v.isReal() || isSig(v) || asEnv(v)) return v;
	if (v.isList()) {
		List* list = (List*)v.o();
		if (!list->isFinite()) indefiniteOp(argName(u, i).c_str(), "");
		return V(list->pack(th));
	}
	wrongType(argName(u, i).c_str(), "Real, UGen or list", v);
}

int64_t itemCount(Arg v) { return v.isList() ? ((List*)v.o())->mArray->size() : 1; }

V item(Arg v, int64_t i) { return v.isList() ? ((List*)v.o())->mArray->at(i) : v; }

// sclang's asAudioRateInput: a constant becomes DC.ar, a control-rate UGen K2A.ar.
V audioRate(Arg v)
{
	if (v.isReal()) return makeUGen("DC", rateAR, {v});
	if (rateOf(v) != rateAR) return makeUGen("K2A", rateAR, {v});
	return v;
}

// SC's multichannel expansion: a list input makes one UGen per item, wrapping shorter lists.
V expand(Thread& th, int row, std::vector<V> const& in, std::vector<bool> const& audio, int numOutputs)
{
	const SpecUGen& u = kSpecUGens[row];
	int64_t n = 0;
	for (Arg v : in)
		if (v.isList()) n = std::max(n, itemCount(v));
	if (n == 0) {
		for (Arg v : in)
			if (v.isList()) {
				post("%s : empty list\n", wordName(u).c_str());
				throw errOutOfRange;
			}
		std::vector<V> inputs = in;
		bool raised = false;
		for (size_t i = 0; i < inputs.size(); ++i)
			if (audio[i]) inputs[i] = audioRate(inputs[i]), raised = true;
		P<UGenNode> node = new UGenNode(u.cls, u.rate, inputs, numOutputs, u.special, u.pure);
		if (raised) node->krInputs = in;
		node->fixedRate = !(u.rates & (1 << rateKR));
		node->spec = row;
		return outputsOf(node);
	}
	P<List> out = new List(itemTypeV, n);
	for (int64_t i = 0; i < n; ++i) {
		std::vector<V> one;
		for (Arg v : in) one.push_back(v.isList() ? item(v, i % itemCount(v)) : v);
		out->add(expand(th, row, one, audio, numOutputs));
	}
	return out;
}

// Lays the arguments out in wire order, then expands.
V build(Thread& th, int row, std::vector<V> const& args)
{
	const SpecUGen& u = kSpecUGens[row];
	// Checked first: LocalIn's wire repeats its default numChannels times.
	int numOutputs = u.outputs;
	if (u.outputsKind == outputsSize) {
		numOutputs = (int)itemCount(args[u.outputsArg]);
	} else if (u.outputsKind == outputsValue) {
		Arg n = args[u.outputsArg];
		if (!n.isReal() || n.f < 1. || n.f != std::floor(n.f) || n.f > 1024.)
			wrongType(argName(u, u.outputsArg).c_str(), "a whole number of channels from 1 to 1024", n);
		numOutputs = (int)n.f;
	}
	std::vector<V> in;
	std::vector<bool> audio;
	for (int w = 0; w < u.numWire; ++w) {
		const SpecWire& sw = u.wire[w];
		Arg a = args[sw.arg];
		if (asEnv(a) && sw.kind != wireEnv && sw.kind != wireEnvInterp)
			wrongType(argName(u, sw.arg).c_str(), "Real, UGen or list", a);
		switch (sw.kind) {
			case wireEnv :
			case wireEnvInterp : {
				// An Env, or a list already in SC's envelope format.
				std::vector<V> env;
				if (Env* e = asEnv(a))
					env = sw.kind == wireEnv ? e->envGenArray(argName(u, sw.arg).c_str())
					                         : e->interpolationArray(th, argName(u, sw.arg).c_str());
				else if (a.isList())
					for (int64_t i = 0; i < itemCount(a); ++i) env.push_back(item(a, i));
				else
					wrongType(argName(u, sw.arg).c_str(), "Env or list", a);
				for (Arg v : env) in.push_back(checkArg(th, u, sw.arg, v));
				break;
			}
			case wireArg :
				in.push_back(a);
				audio.resize(in.size(), u.rate == rateAR && (u.audio >> sw.arg) & 1);
				break;
			case wireSplice :
				for (int64_t i = 0; i < itemCount(a); ++i) in.push_back(item(a, i));
				break;
			case wireSize : in.push_back((double)itemCount(a)); break;
			case wireFlop : {
				// sclang's [freqs, amps, times].flop.flat: one input per row per partial. A row may be a
				// list or a real; an omitted row takes its fill value.
				int64_t rows = itemCount(a);
				if (!a.isList() || rows > 1 + u.numFill) {
					post("%s : expected a list of 1 to %d rows\n", argName(u, sw.arg).c_str(), 1 + u.numFill);
					throw errOutOfRange;
				}
				std::vector<V> lines;
				for (int64_t r = 0; r < rows; ++r) lines.push_back(checkArg(th, u, sw.arg, item(a, r)));
				int64_t n = 0;
				for (Arg line : lines) n = std::max(n, itemCount(line));
				for (int64_t p = 0; p < n; ++p)
					for (int r = 0; r <= u.numFill; ++r)
						in.push_back(r < rows ? item(lines[r], p % itemCount(lines[r])) : V(u.fill[r - 1]));
				break;
			}
			case wireSpliceWrap : {
				// LocalIn's default.wrapExtend(numChannels)
				Arg n = args[sw.sizeArg];
				int64_t count = n.isReal() ? (int64_t)n.f : 0;
				for (int64_t i = 0; i < count; ++i) in.push_back(item(a, i % itemCount(a)));
				break;
			}
			case wireSpliceTo : {
				int64_t n = itemCount(args[sw.sizeArg]);
				for (int64_t i = 0; i < n; ++i) in.push_back(i < itemCount(a) ? item(a, i) : V(0.));
				break;
			}
		}
	}
	audio.resize(in.size(), false);
	return expand(th, row, in, audio, numOutputs);
}

V positionalCall(Thread& th, int row)
{
	const SpecUGen& u = kSpecUGens[row];
	std::vector<V> args(u.numArgs);
	for (int i = u.positional - 1; i >= 0; --i) args[i] = checkArg(th, u, i, th.pop());
	for (int i = u.positional; i < u.numArgs; ++i) args[i] = u.args[i].def;
	return build(th, row, args);
}

V keywordCall(Thread& th, int row, Form* form)
{
	const SpecUGen& u = kSpecUGens[row];
	for (Form* f = form; f; f = f->mNextForm()) {
		TableMap* map = f->mTable->mMap();
		for (size_t k = 0; k < map->mSize; ++k) {
			const char* key = ((String*)map->mKeys[k].o())->s;
			int i = 0;
			while (i < u.numArgs && strcmp(key, u.args[i].name)) ++i;
			if (i == u.numArgs) {
				post("%s : no argument named %s\n", wordName(u).c_str(), key);
				throw errNotFound;
			}
		}
	}
	std::vector<V> args(u.numArgs);
	std::vector<bool> given(u.numArgs);
	for (int i = 0; i < u.numArgs; ++i) {
		V v;
		if (form->get(th, V(getsym(u.args[i].name)), v)) {
			args[i] = checkArg(th, u, i, v);
			given[i] = true;
		} else if (u.args[i].required) {
			post("%s : required\n", argName(u, i).c_str());
			throw errNotFound;
		} else {
			args[i] = u.args[i].def;
		}
	}
	for (int i = 0; i < u.numArgs; ++i)
		if (!given[i] && u.args[i].fallback >= 0) args[i] = args[u.args[i].fallback];
	return build(th, row, args);
}

bool isFormList(Thread& th, Arg v)
{
	if (!v.isVList()) return false;
	List* list = (List*)v.o();
	if (!list->isFinite()) return false;
	P<List> packed = list->pack(th);
	if (packed->mArray->size() == 0) return false;
	for (int64_t i = 0; i < packed->mArray->size(); ++i)
		if (!packed->mArray->at(i).isForm()) return false;
	return true;
}

void specUGen_(Thread& th, Prim* prim)
{
	th.push(positionalCall(th, (int)prim->v.f));
}

// A generated word: a Form, or a list of Forms, in place of the arguments is a keyword call.
class UGenPrim : public Prim
{
public:
	UGenPrim(int row)
		: Prim(specUGen_, (double)row, kSpecUGens[row].positional, 1, kSpecUGens[row].word, kSpecUGens[row].help) {}

	virtual void apply(Thread& th) override
	{
		int row = (int)v.f;
		if (th.stackDepth() >= 1 && th.top().isForm()) {
			V form = th.pop();
			th.push(keywordCall(th, row, (Form*)form.o()));
		} else if (th.stackDepth() >= 1 && th.top().isVList() && isFormList(th, th.top())) {
			P<List> forms = ((List*)th.pop().o())->pack(th);
			P<List> out = new List(itemTypeV, forms->mArray->size());
			for (int64_t i = 0; i < forms->mArray->size(); ++i)
				out->add(keywordCall(th, row, (Form*)forms->mArray->at(i).o()));
			th.push(out);
		} else {
			Prim::apply(th);
		}
	}
};

} // namespace


bool Sig::dot(Thread& th, Arg key, V& ioValue)
{
	if (!key.isString()) return false;
	const char* name = ((String*)key.o())->s;
	const SpecUGen* u = node->spec >= 0 ? &kSpecUGens[node->spec] : nullptr;
	if (!strcmp(name, "kr")) {
		if (u && !(u->rates & (1 << rateKR))) {
			post("%s has no kr\n", wordName(*u).c_str());
			throw errFailed;
		}
		ioValue = toKR(V(this));
		return true;
	}
	if (!strcmp(name, "ir")) {
		if (!u || !(u->rates & (1 << rateIR))) {
			post("%s has no ir\n", u ? wordName(*u).c_str() : node->name.c_str());
			throw errFailed;
		}
		ioValue = toIR(V(this));
		return true;
	}
	if (!strcmp(name, "poll")) {
		// SC's UGen.poll: Poll at 10 Hz, labelled UGen(Class), at the input's rate, ir as kr. It
		// returns its input; the Poll rides on the input's node.
		int rate = std::max(node->rate, (int)rateKR);
		V trig = makeUGen("Impulse", rate, {10., 0.}, 1, 0, true, true);
		std::string label = "UGen(" + node->name + ")";
		std::vector<V> inputs = {trig, V(this), -1., (double)label.size()};
		for (char c : label) inputs.push_back((double)(unsigned char)c);
		V poll = makeUGen("Poll", rate, inputs, 1, 0, false, true);
		node->riders.push_back(((Sig*)poll.o())->node);
		ioValue = V(this);
		return true;
	}
	if (!strcmp(name, "done")) {
		if (!u || u->doneInput < 0) {
			post("%s has no doneAction\n", node->name.c_str());
			throw errFailed;
		}
		// The compiler builds a .kr node from its original's inputs, so .done goes to the original.
		UGenNode* target = node->krOf ? node->krOf() : node();
		if (!target->doneCopy) {
			P<UGenNode> copy = new UGenNode(target->name.c_str(), target->rate, target->inputs, target->numOutputs,
				target->special, target->pure);
			copy->inputs[u->doneInput] = 2.;
			copy->fixedRate = target->fixedRate;
			copy->spec = target->spec;
			copy->krInputs = target->krInputs;
			if (!copy->krInputs.empty()) copy->krInputs[u->doneInput] = 2.;
			target->doneCopy = copy;
		}
		ioValue = new Sig(target->doneCopy, index);
		if (node->krOf) ioValue = node->rate == rateIR ? toIR(ioValue) : toKR(ioValue);
		return true;
	}
	return false;
}

bool isUGenWord(Arg v) { return v.o() && dynamic_cast<UGenPrim*>(v.o()) != nullptr; }

void AddSpecUGenOps();
void AddSpecUGenOps()
{
	// SC has one flat class namespace; its help categories are data, kept as help sections.
	P<Namespace> sc = new Namespace("sc");
	for (int row = 0; row < kNumSpecUGens; ++row)
		sc->mWords[kSpecUGens[row].word] = new UGenPrim(row);
	// Help; the generator emits rows grouped by category, uncategorised ones first.
	for (int row = 0; row < kNumSpecUGens; ++row) {
		const SpecUGen& u = kSpecUGens[row];
		if (row == 0 || strcmp(u.ns, kSpecUGens[row - 1].ns))
			vm.addBifHelp(std::string("\n*** sc") + (*u.ns ? std::string(": ") + u.ns : "") + " ***");
		vm.addBifHelp(wordName(u).c_str(), nullptr, u.help);
	}
	vm.addBifHelp(std::string("\n*** namespaces ***"));
	vm.def("sc", V(sc));
	vm.addBifHelp("sc", nullptr,
		"(--> namespace) SuperCollider 3.14.1's UGens, each its class name in lower case. import sc.* binds them.");
}
