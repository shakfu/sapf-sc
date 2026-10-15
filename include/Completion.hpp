//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

// REPL completion and hints (docs/dev/stac.md, Completion). Names are read from the running
// interpreter, so they follow renamed words and the user's bindings.

#ifndef SAPF_COMPLETION_HPP
#define SAPF_COMPLETION_HPP

#include "VM.hpp"
#include <string>
#include <vector>

// The word being typed at the end of input: a name, or NS.PREFIX, or .PREFIX after a value.
std::string completionContext(const std::string& input);

// Sorted completions for a context:
// - NAMESPACE.prefix: the namespace's words;
// - VALUE.prefix or .prefix: the method names of stac's objects;
// - prefix: builtins, the user's bindings, and namespaced words as namespacedWords names them.
std::vector<std::string> completions(Thread& th, const std::string& context);

// The stack effect in a word's help, "(a b --> c)", or "" when the word is unknown or has none.
std::string stackEffect(Thread& th, const std::string& word);

// NAMESPACE.word for each namespace holding word; ug.word only when no other namespace does, and
// sapf.word only when neither does.
std::vector<std::string> namespacedWords(Thread& th, const std::string& word);

#endif
