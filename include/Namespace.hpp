//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// A named group of words (docs/dev/stac.md, Namespaces). Dot applies a word without passing the
// namespace; comma pushes it.

#ifndef SAPF_NAMESPACE_HPP
#define SAPF_NAMESPACE_HPP

#include "VM.hpp"
#include <map>
#include <string>

class Namespace : public Object
{
public:
	std::string mName;
	std::map<std::string, V> mWords;

	explicit Namespace(const char* name) : mName(name) {}

	virtual const char* TypeName() const override { return "Namespace"; }

	virtual bool get(Thread& th, Arg key, V& value) const override
	{
		if (!key.isString()) return false;
		auto it = mWords.find(((String*)key.o())->s);
		if (it == mWords.end()) return false;
		value = it->second;
		return true;
	}

	virtual V mustGet(Thread& th, Arg key) const override
	{
		V value;
		if (!get(th, key, value)) notFound(key);
		return value;
	}

	virtual bool dot(Thread& th, Arg key, V& ioValue) override
	{
		V value;
		if (!get(th, key, value)) return false;
		if (!value.isFunOrPrim()) {
			ioValue = value;
			return true;
		}
		value.apply(th);
		ioValue = th.pop();
		return true;
	}

	using Object::print;
	virtual void print(Thread& th, std::string& out, int depth) override
	{
		out += "Namespace " + mName + " (" + std::to_string(mWords.size()) + " words)";
	}

	// Adds a primitive word and its help line, listed as name.word.
	void addPrim(const char* word, int takes, PrimFun fun, const char* help)
	{
		mWords[word] = V(new Prim(fun, 0., takes, 1, word, help));
		vm.addBifHelp((mName + "." + word).c_str(), nullptr, help);
	}
};

// A namespace of generated UGen words, by name, so interpreter words can join it (SpecUGens.cpp).
// nullptr if there is none.
Namespace* generatedNamespace(const std::string& name);

#endif
