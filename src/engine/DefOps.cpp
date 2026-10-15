//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// synthdef, which the def statement (Parser.cpp) compiles to, and node IDs. Def and Node are in
// Def.hpp.

#include "Def.hpp"
#include "Graph.hpp"
#include "Namespace.hpp"
#include "symbol.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace {

int gNextNode = 3000000 + (int)(getpid() % 1000) * 100000;

} // namespace

int newNodeID() { return gNextNode++; }

void checkServer(const std::string& err, const char* word)
{
	if (err.empty()) return;
	post("%s : scsynth at %s: %s. Start one with 'make server' or set SAPF_SCSYNTH.\n", word,
		scsynthAddress().c_str(), err.c_str());
	throw errFailed;
}

std::vector<std::pair<std::string, V>> formPairs(Thread& th, Form* form)
{
	std::vector<std::pair<std::string, V>> out;
	for (Form* f = form; f; f = f->mNextForm()) {
		TableMap* map = f->mTable->mMap();
		for (size_t k = 0; k < map->mSize; ++k) {
			std::string key = ((String*)map->mKeys[k].o())->s;
			bool shadowed = false;
			for (auto& kv : out) shadowed |= kv.first == key;
			if (!shadowed) out.push_back({key, f->mTable->mValues[k]});
		}
	}
	return out;
}

namespace {

// Restores gDefRoots when the body returns or throws.
struct DefRootsScope {
	std::vector<P<UGenNode>>* saved;
	explicit DefRootsScope(std::vector<P<UGenNode>>* roots) : saved(gDefRoots) { gDefRoots = roots; }
	~DefRootsScope() { gDefRoots = saved; }
};

int kindOf(const std::string& name)
{
	if (name.rfind("t_", 0) == 0) return paramTR;
	if (name.rfind("i_", 0) == 0) return paramIR;
	if (name.rfind("a_", 0) == 0) return paramAR;
	return paramKR;
}

// (fun defaults name --> Def): fun's argument names are the controls, defaults a Form of their
// defaults (0 where absent). The def statement compiles to this.
void synthdef_(Thread& th, Prim* prim)
{
	V name = th.pop();
	if (!name.isString()) wrongType("synthdef : name", "String", name);
	V defaults = th.pop();
	if (!defaults.isForm()) wrongType("synthdef : defaults", "Form", defaults);
	V fun = th.pop();
	if (!fun.isFun()) wrongType("synthdef : fun", "Function", fun);
	FunDef* fd = ((Fun*)fun.o())->mDef();

	DefParts parts;
	P<Def> def = new Def();
	def->mName = ((String*)name.o())->s;
	V outParam;
	bool haveOut = false;
	for (size_t i = 0; i < fd->mArgNames.size(); ++i) {
		std::string arg = fd->mArgNames[i]->s;
		V value;
		double d = 0.;
		if (((Form*)defaults.o())->get(th, V(fd->mArgNames[i]), value)) {
			if (!value.isReal()) wrongType(("synthdef : " + arg).c_str(), "Real", value);
			d = value.f;
		}
		V p = makeParam(arg.c_str(), d, kindOf(arg), (int)i);
		parts.params.push_back(p);
		def->mParams.push_back(arg);
		if (arg == "out") outParam = p, haveOut = true;
		th.push(p);
	}
	size_t base = th.stackDepth() - fd->mArgNames.size();
	{
		DefRootsScope scope(&parts.roots);
		fun.apply(th);
	}
	std::vector<V> results;
	while (th.stackDepth() > base) results.insert(results.begin(), th.pop());
	V channels;
	if (results.size() == 1) {
		channels = results[0];
	} else {
		P<List> list = new List(itemTypeV, (int64_t)results.size());
		for (Arg r : results) list->add(r);
		channels = V(list);
	}
	// SC's Function.play writes the channels to a control out, default 0.
	if (!results.empty() && !haveOut) {
		outParam = makeParam("out", 0., paramKR, (int)fd->mArgNames.size());
		parts.params.push_back(outParam);
		def->mParams.push_back("out");
	}
	if (!results.empty()) parts.outBus = outParam;
	def->mBytes = compileDef(th, results.empty() ? V(new List(itemTypeV, 0)) : channels, def->mName, parts);
	th.push(V(def));
}

// (placement --> group): SC's Group(target, addAction), the Form taking :target and :addAction.
void group_(Thread& th, Prim* prim)
{
	V form = th.pop();
	if (!form.isForm()) wrongType("node.group : placement", "Form", form);
	int target = 0, addAction = 0;
	for (auto& [k, v] : formPairs(th, (Form*)form.o()))
		if (!placement("node.group", k, v, target, addAction)) {
			post("node.group : no field %s; it takes :target and :addAction\n", k.c_str());
			throw errNotFound;
		}
	int id = newNodeID();
	checkServer(scsynthSendSync({oscMessage("/g_new", {{OscArg::Int, id, ""}, {OscArg::Int, addAction, ""},
		{OscArg::Int, target, ""}})}), "node.group");
	th.push(V(new Node(id, true)));
}

} // namespace

void AddDefOps();
void AddDefOps()
{
	vm.addBifHelp(std::string("\n*** defs and nodes ***"));
	vm.def("synthdef", 3, 1, synthdef_,
		"(fun defaults name --> def) compiles a SynthDef. fun's arguments are its controls; t_, i_ and a_ make "
		"trigger, scalar and audio controls. Channels left by fun go to a control out. Call the def with a Form to "
		"start a synth; .add sends it, .write writes name.scsyndef.");

	// node's UGen words are generated (SpecUGens.cpp); group joins them.
	Namespace* node = generatedNamespace("node");
	if (!node) {
		post("AddDefOps : no node namespace\n");
		throw errFailed;
	}
	node->addPrim("group", 1, group_,
		"(placement --> group) a new group; placement is a Form taking :target and :addAction, {} for the head of "
		"the root. A group is a target, and set, release and free act on every synth in it.");
}
