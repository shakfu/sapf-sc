//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// The env namespace: SC's Env constructors, with SC's defaults and array formats
// (SCClassLibrary/Common/Audio/Env.sc).

#include "Env.hpp"
#include "Graph.hpp"
#include "Namespace.hpp"
#include <cstring>

namespace {

// SC's Env.shapeNames; a real or a UGen is curvature, shape 5.
int shapeNumber(Arg curve, const char* msg)
{
	if (!curve.isString()) return 5;
	static const struct { const char* name; int shape; } names[] = {
		{"step", 0}, {"lin", 1}, {"linear", 1}, {"exp", 2}, {"exponential", 2}, {"sin", 3}, {"sine", 3},
		{"wel", 4}, {"welch", 4}, {"sqr", 6}, {"squared", 6}, {"cub", 7}, {"cubed", 7}, {"hold", 8}};
	const char* s = ((String*)curve.o())->s;
	for (auto& n : names)
		if (!strcmp(s, n.name)) return n.shape;
	post("%s : no envelope shape named '%s\n", msg, s);
	throw errOutOfRange;
}

V curveValue(Arg curve) { return curve.isString() ? V(0.) : curve; }

} // namespace

void Env::print(Thread& th, std::string& out, int depth)
{
	auto list = [&](std::vector<V> const& vs) {
		out += "[";
		for (size_t i = 0; i < vs.size(); ++i) {
			if (i) out += " ";
			if (vs[i].isString()) out += "'";
			vs[i].print(th, out, depth + 1);
		}
		out += "]";
	};
	out += "Env(";
	list(levels);
	out += " ";
	list(times);
	out += " ";
	list(curves);
	out += ")";
}

std::vector<V> Env::envGenArray(const char* msg) const
{
	std::vector<V> a = {levels[0], (double)times.size(), releaseNode, loopNode};
	for (size_t i = 0; i < times.size(); ++i) {
		Arg c = curves[i % curves.size()];
		a.insert(a.end(), {levels[i + 1], times[i], (double)shapeNumber(c, msg), curveValue(c)});
	}
	return a;
}

std::vector<V> Env::interpolationArray(Thread& th, const char* msg) const
{
	V total = times[0];
	for (size_t i = 1; i < times.size(); ++i) total = graphBinary(th, opAdd, total, times[i]);
	std::vector<V> a = {offset, levels[0], (double)times.size(), total};
	for (size_t i = 0; i < times.size(); ++i) {
		Arg c = curves[i % curves.size()];
		a.insert(a.end(), {times[i], (double)shapeNumber(c, msg), curveValue(c), levels[i + 1]});
	}
	return a;
}

namespace {

// A level, time or curvature: a real, a UGen, or a list of them for several channels.
V popPoint(Thread& th, const char* msg)
{
	V v = th.pop();
	if (v.isRef() || v.isZRef()) v = v.deref();
	if (v.isList()) {
		if (!((List*)v.o())->isFinite()) indefiniteOp(msg, "");
		return V(((List*)v.o())->pack(th));
	}
	if (!v.isReal() && !isSig(v)) wrongType(msg, "Real, UGen or list", v);
	return v;
}

V popCurve(Thread& th, const char* msg)
{
	V v = th.pop();
	if (v.isString()) {
		shapeNumber(v, msg);
		return v;
	}
	th.push(v);
	return popPoint(th, msg);
}

std::vector<V> items(Thread& th, Arg v)
{
	if (!v.isList()) return {v};
	P<List> list = ((List*)v.o())->pack(th);
	std::vector<V> out;
	for (int64_t i = 0; i < list->mArray->size(); ++i) out.push_back(list->mArray->at(i));
	return out;
}

void pushEnv(Thread& th, std::vector<V> levels, std::vector<V> times, std::vector<V> curves, double releaseNode = -99.)
{
	P<Env> env = new Env();
	env->levels = std::move(levels);
	env->times = std::move(times);
	env->curves = std::move(curves);
	env->releaseNode = releaseNode;
	th.push(V(env));
}

void perc_(Thread& th, Prim* prim)
{
	V rel = popPoint(th, "env.perc : releaseTime");
	V atk = popPoint(th, "env.perc : attackTime");
	pushEnv(th, {0., 1., 0.}, {atk, rel}, {-4.});
}

void adsr_(Thread& th, Prim* prim)
{
	V rel = popPoint(th, "env.adsr : releaseTime");
	V sus = popPoint(th, "env.adsr : sustainLevel");
	V dec = popPoint(th, "env.adsr : decayTime");
	V atk = popPoint(th, "env.adsr : attackTime");
	pushEnv(th, {0., 1., sus, 0.}, {atk, dec, rel}, {-4.}, 2.);
}

void asr_(Thread& th, Prim* prim)
{
	V rel = popPoint(th, "env.asr : releaseTime");
	V atk = popPoint(th, "env.asr : attackTime");
	pushEnv(th, {0., 1., 0.}, {atk, rel}, {-4.}, 1.);
}

void linen_(Thread& th, Prim* prim)
{
	V rel = popPoint(th, "env.linen : releaseTime");
	V sus = popPoint(th, "env.linen : sustainTime");
	V atk = popPoint(th, "env.linen : attackTime");
	pushEnv(th, {0., 1., 1., 0.}, {atk, sus, rel}, {V(getsym("lin"))});
}

// SC's Env.new: times wrap to one per segment; curves wrap per segment when read.
void new_(Thread& th, Prim* prim)
{
	V curves = popCurve(th, "env.new : curves");
	V times = popPoint(th, "env.new : times");
	V levels = popPoint(th, "env.new : levels");
	std::vector<V> ls = items(th, levels), ts = items(th, times), cs = items(th, curves);
	if (ls.size() < 2 || ts.empty() || cs.empty()) {
		post("env.new : needs 2 or more levels, and times and curves\n");
		throw errOutOfRange;
	}
	for (Arg c : cs)
		if (c.isString()) shapeNumber(c, "env.new : curves");
	std::vector<V> segs;
	for (size_t i = 0; i + 1 < ls.size(); ++i) segs.push_back(ts[i % ts.size()]);
	pushEnv(th, ls, segs, cs);
}

} // namespace

void AddEnvOps();
void AddEnvOps()
{
	vm.addBifHelp(std::string("\n*** env ***"));
	P<Namespace> env = new Namespace("env");
	env->addPrim("perc", 2, perc_, "(attackTime releaseTime --> env) SC's Env.perc: rises to 1, falls to 0, curve -4.");
	env->addPrim("adsr", 4, adsr_,
		"(attackTime decayTime sustainLevel releaseTime --> env) SC's Env.adsr: holds sustainLevel until the gate closes.");
	env->addPrim("asr", 2, asr_, "(attackTime releaseTime --> env) SC's Env.asr: holds 1 until the gate closes.");
	env->addPrim("linen", 3, linen_, "(attackTime sustainTime releaseTime --> env) SC's Env.linen: linear, holds 1.");
	env->addPrim("new", 3, new_,
		"(levels times curves --> env) SC's Env.new. curves: a real, a UGen, a shape such as 'lin 'exp 'sin, or a list.");
	vm.def("env", V(env));
	vm.addBifHelp("env", nullptr, "(--> namespace) envelopes for envgen and ienvgen.");
}
