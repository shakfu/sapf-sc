//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include "Highlight.hpp"
#include "StacHighlights.h"
#include "Graph.hpp"
#include "Namespace.hpp"
#include "VM.hpp"
#include <cstring>
#include <regex>
#include <set>
#include <string>
#include <tree_sitter/api.h>

extern "C" const TSLanguage* tree_sitter_stac(void);

namespace {

// nvim-treesitter capture names, most specific first.
Hl kindOf(std::string_view name)
{
	static const std::pair<const char*, Hl> kinds[] = {
		{"comment", Hl::Comment}, {"string.special.symbol", Hl::Symbol}, {"string", Hl::String},
		{"number", Hl::Number}, {"keyword", Hl::Keyword}, {"operator", Hl::Operator},
		{"punctuation.bracket", Hl::Bracket}, {"punctuation.delimiter", Hl::Delimiter},
		{"function.method.call", Hl::Method}, {"function.call", Hl::Word}, {"function", Hl::Definition},
		{"constant", Hl::Definition}, {"variable.parameter", Hl::Parameter}, {"variable", Hl::Variable},
		{"type", Hl::Type}, {"module", Hl::Module}, {"property", Hl::Property},
	};
	for (auto& [prefix, kind] : kinds)
		if (name.compare(0, strlen(prefix), prefix) == 0) return kind;
	return Hl::Plain;
}

// A text predicate of a pattern: #match?, #not-match?, #eq? or #not-eq? on a capture.
struct Predicate {
	uint32_t capture;
	bool negate;
	bool regex;
	std::string text;
	std::regex re;
};

struct Highlighter {
	TSParser* parser = ts_parser_new();
	TSQuery* query = nullptr;
	std::vector<Hl> kinds;                           // by capture index
	std::vector<std::vector<Predicate>> predicates;  // by pattern index

	Highlighter()
	{
		ts_parser_set_language(parser, tree_sitter_stac());
		uint32_t offset;
		TSQueryError error;
		query = ts_query_new(tree_sitter_stac(), kStacHighlights, (uint32_t)strlen(kStacHighlights), &offset, &error);
		if (!query) return;
		for (uint32_t i = 0; i < ts_query_capture_count(query); ++i) {
			uint32_t len;
			const char* name = ts_query_capture_name_for_id(query, i, &len);
			kinds.push_back(kindOf(std::string_view(name, len)));
		}
		for (uint32_t p = 0; p < ts_query_pattern_count(query); ++p) {
			uint32_t n;
			const TSQueryPredicateStep* steps = ts_query_predicates_for_pattern(query, p, &n);
			std::vector<Predicate> preds;
			for (uint32_t i = 0; i < n;) {
				uint32_t end = i;
				while (end < n && steps[end].type != TSQueryPredicateStepTypeDone) ++end;
				// #op? @capture "text"; other predicates are ignored
				if (end - i == 3 && steps[i + 1].type == TSQueryPredicateStepTypeCapture &&
					steps[i + 2].type == TSQueryPredicateStepTypeString) {
					uint32_t len;
					std::string op = ts_query_string_value_for_id(query, steps[i].value_id, &len);
					Predicate pr;
					pr.capture = steps[i + 1].value_id;
					pr.negate = op.rfind("not-", 0) == 0;
					pr.regex = op.find("match") != std::string::npos;
					pr.text = ts_query_string_value_for_id(query, steps[i + 2].value_id, &len);
					if (pr.regex) pr.re = std::regex(pr.text);
					preds.push_back(std::move(pr));
				}
				i = end + 1;
			}
			predicates.push_back(std::move(preds));
		}
	}

	bool passes(TSQueryMatch const& m, std::string_view src) const
	{
		for (Predicate const& pr : predicates[m.pattern_index])
			for (uint16_t i = 0; i < m.capture_count; ++i) {
				if (m.captures[i].index != pr.capture) continue;
				uint32_t s = ts_node_start_byte(m.captures[i].node), e = ts_node_end_byte(m.captures[i].node);
				std::string text(src.substr(s, e - s));
				bool hit = pr.regex ? std::regex_search(text, pr.re) : text == pr.text;
				if (hit == pr.negate) return false;
			}
		return true;
	}
};

Highlighter& highlighter()
{
	static Highlighter h;
	return h;
}

// A word's value, from the workspace or the builtins. A String that is not interned, so a partial word
// typed at the prompt stays out of the symbol table; GTable compares strings by text.
bool lookup(Thread& th, std::string const& word, V& value, bool& inWorkspace)
{
	V key(new String(word.c_str()));
	inWorkspace = th.mWorkspace->get(th, key, value);
	return inWorkspace || vm.builtins->get(th, key, value);
}

// A word Library, UGen, Bound or Unbound. A prelude word rebound since is Bound.
Hl classify(Thread& th, std::string const& word, std::set<std::string> const& local)
{
	if (local.count(word)) return Hl::Bound;
	V value, before;
	bool inWorkspace;
	if (!lookup(th, word, value, inWorkspace)) return Hl::Unbound;
	if (inWorkspace && !(th.mLibrary && th.mLibrary->get(th, V(new String(word.c_str())), before) && before.Identical(value)))
		return Hl::Bound;
	return isUGenWord(value) ? Hl::UGen : Hl::Library;
}

// Whether module.word, such as sc.sinosc, names a UGen.
bool isModuleUGen(Thread& th, std::string const& module, std::string const& word)
{
	V ns, value;
	bool inWorkspace;
	return lookup(th, module, ns, inWorkspace) && ns.o() && dynamic_cast<Namespace*>(ns.o()) &&
		ns.o()->get(th, V(new String(word.c_str())), value) && isUGenWord(value);
}

} // namespace

std::vector<Hl> highlight(std::string_view src, Thread* th)
{
	Highlighter& h = highlighter();
	std::vector<Hl> out(src.size(), Hl::Plain);
	if (!h.query) return out;
	TSTree* tree = ts_parser_parse_string(h.parser, nullptr, src.data(), (uint32_t)src.size());
	TSQueryCursor* cursor = ts_query_cursor_new();
	ts_query_cursor_exec(cursor, h.query, ts_tree_root_node(tree));
	std::vector<uint32_t> pattern(src.size(), UINT32_MAX);
	TSQueryMatch m;
	uint32_t ci;
	while (ts_query_cursor_next_capture(cursor, &m, &ci)) {
		if (!h.passes(m, src)) continue;
		TSQueryCapture c = m.captures[ci];
		uint32_t s = ts_node_start_byte(c.node), e = std::min<uint32_t>(ts_node_end_byte(c.node), (uint32_t)src.size());
		for (uint32_t i = s; i < e; ++i)
			if (m.pattern_index < pattern[i]) pattern[i] = m.pattern_index, out[i] = h.kinds[c.index];
	}
	ts_query_cursor_delete(cursor);
	ts_tree_delete(tree);
	if (!th) return out;

	// Runs of one kind are single tokens: words never touch without a separator.
	auto runs = [&](auto f) {
		for (size_t i = 0; i < out.size();) {
			size_t j = i;
			while (j < out.size() && out[j] == out[i]) ++j;
			f(i, j);
			i = j;
		}
	};
	std::set<std::string> local;
	runs([&](size_t i, size_t j) {
		if (out[i] == Hl::Variable || out[i] == Hl::Parameter || out[i] == Hl::Definition)
			local.insert(std::string(src.substr(i, j - i)));
	});
	size_t wordStart = 0, wordEnd = 0;  // the last word, the receiver of a member such as sc.sinosc
	runs([&](size_t i, size_t j) {
		if (out[i] == Hl::Method && wordEnd > 0 && wordEnd + 1 == i && src[wordEnd] == '.' &&
			isModuleUGen(*th, std::string(src.substr(wordStart, wordEnd - wordStart)), std::string(src.substr(i, j - i))))
			std::fill(out.begin() + i, out.begin() + j, Hl::UGen);
		if (out[i] != Hl::Word) return;
		wordStart = i, wordEnd = j;
		Hl k = classify(*th, std::string(src.substr(i, j - i)), local);
		std::fill(out.begin() + i, out.begin() + j, k);
	});
	return out;
}

bool parsesCleanly(std::string_view src)
{
	Highlighter& h = highlighter();
	TSTree* tree = ts_parser_parse_string(h.parser, nullptr, src.data(), (uint32_t)src.size());
	bool clean = !ts_node_has_error(ts_tree_root_node(tree));
	ts_tree_delete(tree);
	return clean;
}
