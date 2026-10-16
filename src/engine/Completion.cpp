//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Completion.hpp"
#include "Namespace.hpp"
#include "symbol.hpp"
#include <algorithm>
#include <cstring>
#include <set>

namespace {

// Characters that end a name, as in Parser.cpp; '.' is kept in a context to split it.
bool endsName(char c) { return isspace((unsigned char)c) || strchr(";()[]{}`,:\"", c); }

// The methods of Sig, Def, Node, Bus, Player, Clock and Ndef.
const char* const kMethods[] = {"add", "ar", "done", "end", "free", "ir", "kr", "pdef", "play", "poll", "release", "releaseIn", "set", "stop", "write"};

bool startsWith(const std::string& s, const std::string& prefix) { return s.compare(0, prefix.size(), prefix) == 0; }

std::string keyOf(Arg key) { return key.isString() ? ((String*)key.o())->s : std::string(); }

// Calls f(name, value) for every builtin and user binding.
template <class F>
void eachGlobal(Thread& th, F f)
{
	for (P<TreeNode> const& n : vm.builtins->sorted()) f(keyOf(n->mKey), n->mValue);
	for (GForm* form = th.mWorkspace(); form; form = form->mNextForm())
		for (P<TreeNode> const& n : form->mTable->sorted()) f(keyOf(n->mKey), n->mValue);
}

Namespace* asNamespace(Arg v) { return v.o() ? dynamic_cast<Namespace*>(v.o()) : nullptr; }

// The builtin namespaces, by their global names.
std::vector<std::pair<std::string, Namespace*>> namespaces()
{
	std::vector<std::pair<std::string, Namespace*>> out;
	for (P<TreeNode> const& n : vm.builtins->sorted())
		if (Namespace* ns = asNamespace(n->mValue)) out.push_back({keyOf(n->mKey), ns});
	return out;
}

std::vector<std::string> namespaced(std::vector<std::pair<std::string, Namespace*>> const& all, const std::string& word)
{
	std::vector<std::string> out;
	bool inSc = false, inSapf = false;
	for (auto& [name, ns] : all)
		if (ns->mWords.count(word)) {
			if (ns->mName == "sc") inSc = true;
			else if (ns->mName == "sapf") inSapf = true;
			else if (ns->mName != "stac") out.push_back(name + "." + word);  // stac holds every core word
		}
	// SC's UGens after interpreter words; sapf's words are the compatibility library and come last.
	if (out.empty() && inSc) out.push_back("sc." + word);
	if (out.empty() && inSapf) out.push_back("sapf." + word);
	return out;
}

Namespace* namespaceNamed(Thread& th, const std::string& name)
{
	V v;
	if (th.mWorkspace->get(th, V(getsym(name.c_str())), v) || vm.builtins->get(th, V(getsym(name.c_str())), v))
		return asNamespace(v);
	return nullptr;
}

} // namespace

std::string completionContext(const std::string& input)
{
	size_t i = input.size();
	while (i > 0 && !endsName(input[i - 1])) --i;
	std::string context = input.substr(i);
	// a binding, symbol or lambda parameter prefix is not part of the name
	if (!context.empty() && strchr("='\\", context[0])) context.erase(0, 1);
	return context;
}

std::vector<std::string> completions(Thread& th, const std::string& context)
{
	std::set<std::string> out;
	size_t dot = context.rfind('.');
	if (dot != std::string::npos) {
		std::string head = context.substr(0, dot + 1), prefix = context.substr(dot + 1);
		if (Namespace* ns = namespaceNamed(th, context.substr(0, dot))) {
			for (auto& [word, value] : ns->mWords)
				if (startsWith(word, prefix)) out.insert(head + word);
		} else {
			for (const char* m : kMethods)
				if (startsWith(m, prefix)) out.insert(head + m);
		}
		return {out.begin(), out.end()};
	}
	eachGlobal(th, [&](const std::string& name, Arg value) {
		if (!name.empty() && startsWith(name, context)) out.insert(name);
	});
	auto all = namespaces();
	std::set<std::string> words;
	for (auto& [name, ns] : all)
		for (auto it = ns->mWords.lower_bound(context); it != ns->mWords.end() && startsWith(it->first, context); ++it)
			words.insert(it->first);
	for (std::string const& word : words)
		for (std::string const& w : namespaced(all, word)) out.insert(w);
	return {out.begin(), out.end()};
}

std::string stackEffect(Thread& th, const std::string& word)
{
	V value;
	size_t dot = word.rfind('.');
	if (dot == std::string::npos) {
		P<String> key = getsym(word.c_str());
		if (!th.mWorkspace->get(th, V(key), value) && !vm.builtins->get(th, V(key), value)) return "";
	} else {
		Namespace* ns = namespaceNamed(th, word.substr(0, dot));
		if (!ns) return "";
		auto it = ns->mWords.find(word.substr(dot + 1));
		if (it == ns->mWords.end()) return "";
		value = it->second;
	}
	const char* help = value.OneLineHelp();
	if (!help) return "";
	const char* open = strchr(help, '(');
	const char* close = open ? strchr(open, ')') : nullptr;
	return close ? std::string(open, close + 1) : "";
}

std::vector<std::string> namespacedWords(Thread& th, const std::string& word) { return namespaced(namespaces(), word); }
