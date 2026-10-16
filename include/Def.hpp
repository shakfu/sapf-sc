//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// stac's Def and Node (docs/dev/stac.md, Definitions and Server objects). A Def is a compiled
// SynthDef; calling it with a Form starts a synth and pushes its Node.

#ifndef SAPF_DEF_HPP
#define SAPF_DEF_HPP

#include "Resources.hpp"
#include "Environment.hpp"
#include "Scsynth.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

// Throws, naming the server, when a send failed.
void checkServer(const std::string& err, const char* word);

// The symbol's text, or nullptr.
inline const char* symbolOf(Arg v) { return v.isString() ? ((String*)v.o())->s : nullptr; }

// The key and value pairs of a Form, its own and inherited.
std::vector<std::pair<std::string, V>> formPairs(Thread& th, Form* form);

// A node ID from this process's block of 100000, above play's 2000000.. and sclang's 1000.., so a
// second sapf on the same server, or a restarted one, does not reuse the IDs of synths still running.
int newNodeID();

// Reads :target or :addAction from a Form's pair into target and addAction. False for other keys.
inline bool placement(std::string const& word, std::string const& key, Arg v, int& target, int& addAction)
{
	if (key == "target") {
		if (!v.isReal() && !asResource(v)) wrongType((word + " : target").c_str(), "Real or Node", v);
		target = (int)v.asFloat();
		return true;
	}
	if (key != "addAction") return false;
	const char* s = symbolOf(v);
	int i = 0;
	static const char* const actions[] = {"head", "tail", "before", "after", "replace"};
	while (s && i < 5 && strcmp(s, actions[i])) ++i;
	if (!s || i == 5) wrongType((word + " : addAction").c_str(), "'head 'tail 'before 'after or 'replace", v);
	addAction = i;
	return true;
}

class Node : public ServerResource
{
public:
	bool mGroup;

	explicit Node(int id, bool group = false) : ServerResource(id), mGroup(group) {}

	virtual const char* TypeName() const override { return mGroup ? "Group" : "Node"; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += (mGroup ? "Group " : "Node ") + std::to_string(mIndex);
	}

	void nset(std::vector<OscArg> pairs, const char* word)
	{
		pairs.insert(pairs.begin(), {OscArg::Int, mIndex, ""});
		checkServer(scsynthSendSync({oscMessage("/n_set", pairs)}), word);
	}

	// set, release, releaseIn and free change the synth and push nothing.
	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (!name) return false;
		if (!strcmp(name, "set")) {
			V form = th.pop();
			if (!form.isForm()) wrongType("set : controls", "Form", form);
			std::vector<OscArg> pairs;
			for (auto& [k, v] : formPairs(th, (Form*)form.o())) {
				if (!v.isReal() && !asResource(v)) wrongType(("set : " + k).c_str(), "Real, Bus, Buffer or Node", v);
				pairs.push_back({OscArg::String, 0, k});
				pairs.push_back({OscArg::Float, 0, "", (float)v.asFloat()});
			}
			nset(pairs, "set");
		} else if (!strcmp(name, "release")) {
			nset({{OscArg::String, 0, "gate"}, {OscArg::Float, 0, "", 0.f}}, "release");
		} else if (!strcmp(name, "releaseIn")) {
			// SC's release(t): a gate of -1 - t makes EnvGen release over t seconds.
			double t = th.popFloat("releaseIn : seconds");
			nset({{OscArg::String, 0, "gate"}, {OscArg::Float, 0, "", (float)(-1. - t)}}, "releaseIn");
		} else if (!strcmp(name, "free")) {
			checkServer(scsynthSendSync({oscMessage("/n_free", {{OscArg::Int, mIndex, ""}})}), "free");
		} else {
			return false;
		}
		ioValue = noValue();
		return true;
	}
};

class Def : public Object
{
public:
	std::string mName;
	std::string mBytes;
	std::vector<std::string> mParams;
	bool mSent = false;

	virtual const char* TypeName() const override { return "SynthDef"; }

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += "SynthDef " + mName + " (";
		for (size_t i = 0; i < mParams.size(); ++i) out += (i ? " " : "") + mParams[i];
		out += ")";
	}

	std::string recv(const std::string& completion = "")
	{
		std::vector<OscArg> args = {{OscArg::Blob, 0, mBytes}};
		if (!completion.empty()) args.push_back({OscArg::Blob, 0, completion});
		return oscMessage("/d_recv", args);
	}

	// {...} NAME: starts a synth with the Form's controls. :target and :addAction place it.
	virtual void apply(Thread& th) override
	{
		V form = th.pop();
		if (!form.isForm()) wrongType((mName + " : controls").c_str(), "Form", form);
		int target = 0, addAction = 0;
		std::vector<OscArg> pairs;
		for (auto& [k, v] : formPairs(th, (Form*)form.o())) {
			std::string msg = mName + " : " + k;
			if (placement(mName, k, v, target, addAction)) {
			} else if (std::find(mParams.begin(), mParams.end(), k) == mParams.end()) {
				post("%s : no control named %s\n", mName.c_str(), k.c_str());
				throw errNotFound;
			} else {
				if (!v.isReal() && !asResource(v)) wrongType(msg.c_str(), "Real, Bus, Buffer or Node", v);
				pairs.push_back({OscArg::String, 0, k});
				pairs.push_back({OscArg::Float, 0, "", (float)v.asFloat()});
			}
		}
		int id = newNodeID();
		std::vector<OscArg> args = {{OscArg::String, 0, mName}, {OscArg::Int, id, ""}, {OscArg::Int, addAction, ""},
			{OscArg::Int, target, ""}};
		args.insert(args.end(), pairs.begin(), pairs.end());
		std::string snew = oscMessage("/s_new", args);
		// sent with its first synth: scsynth starts the synth when the def has loaded
		checkServer(scsynthSendSync({mSent ? snew : recv(snew)}), mName.c_str());
		mSent = true;
		th.push(V(new Node(id)));
	}

	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		const char* name = symbolOf(key);
		if (!name) return false;
		if (!strcmp(name, "add")) {
			checkServer(scsynthSendSync({recv()}), (mName + ".add").c_str());
			mSent = true;
		} else if (!strcmp(name, "write")) {
			const char* dir = stacEnv("SYNTHDEFS");
			std::string path = ((dir && *dir) ? std::string(dir) + "/" : std::string()) + mName + ".scsyndef";
			std::ofstream f(path, std::ios::binary);
			if (!f || !f.write(mBytes.data(), (std::streamsize)mBytes.size())) {
				post("%s.write : could not write '%s'\n", mName.c_str(), path.c_str());
				throw errFailed;
			}
			post("wrote '%s'\n", path.c_str());
		} else {
			return false;
		}
		ioValue = noValue();
		return true;
	}
};


inline Def* asDef(Arg v) { return v.o() ? dynamic_cast<Def*>(v.o()) : nullptr; }

#endif
