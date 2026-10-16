//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// Syntax highlighting of stac source with tree-sitter-stac/ and its queries/highlights.scm.

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

class Thread;

enum class Hl : uint8_t {
	Plain, Comment, String, Number, Keyword, Operator, Bracket, Delimiter, Definition, Parameter, Type, Variable,
	Module, Symbol, Property, Method,
	Word,     // a word, without a Thread to look it up
	Library,  // a word the VM defines, or Thread::mLibrary binds to its current value
	UGen,     // a Library word whose value is a UGen of the namespace sc
	Bound,    // any other word the workspace binds, or src binds as a name or parameter
	Unbound,  // a word nothing binds
};

// One kind per byte of src. Where captures overlap, the earlier pattern in highlights.scm wins. With th,
// each word is Library, UGen, Bound or Unbound instead of Word.
std::vector<Hl> highlight(std::string_view src, Thread* th = nullptr);

// Whether src parses with no ERROR or MISSING node.
bool parsesCleanly(std::string_view src);
