//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "test_common.hpp"
#include "Highlight.hpp"
#include "VM.hpp"
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace fs = std::filesystem;

class HighlightTest : public SapfTestBase {
protected:
    Thread th;

    // The kind of the nth occurrence of token in src; fails if its bytes differ.
    static Hl kindOf(std::vector<Hl> const& kinds, std::string const& src, std::string const& token, int nth = 0) {
        size_t at = src.find(token);
        for (int i = 0; i < nth && at != std::string::npos; ++i) at = src.find(token, at + 1);
        EXPECT_NE(at, std::string::npos) << token;
        if (at == std::string::npos) return Hl::Plain;
        for (size_t i = at; i < at + token.size(); ++i)
            EXPECT_EQ((int)kinds[i], (int)kinds[at]) << token << " at byte " << i;
        return kinds[at];
    }

    static std::string read(fs::path const& p) {
        std::ifstream f(p);
        std::stringstream s;
        s << f.rdbuf();
        return s.str();
    }
};

TEST_F(HighlightTest, SyntaxKinds) {
    std::string src = "def dbl x --> num \"d\" [x 2 * +/ 'sym `dbl f,key {:k 1} sc.sinosc] ; note";
    std::vector<Hl> k = highlight(src);
    ASSERT_EQ(k.size(), src.size());
    EXPECT_EQ(kindOf(k, src, "def"), Hl::Keyword);
    EXPECT_EQ(kindOf(k, src, "dbl"), Hl::Definition);
    EXPECT_EQ(kindOf(k, src, "x"), Hl::Parameter);
    EXPECT_EQ(kindOf(k, src, "-->"), Hl::Operator);
    EXPECT_EQ(kindOf(k, src, "num"), Hl::Type);
    EXPECT_EQ(kindOf(k, src, "\"d\""), Hl::String);
    EXPECT_EQ(kindOf(k, src, "["), Hl::Bracket);
    EXPECT_EQ(kindOf(k, src, "x", 1), Hl::Word);
    EXPECT_EQ(kindOf(k, src, "2"), Hl::Number);
    EXPECT_EQ(kindOf(k, src, "*"), Hl::Operator);  // a #match? predicate
    EXPECT_EQ(kindOf(k, src, "+/"), Hl::Operator);
    EXPECT_EQ(kindOf(k, src, "sym"), Hl::Symbol);
    EXPECT_EQ(kindOf(k, src, "key"), Hl::Property);
    EXPECT_EQ(kindOf(k, src, "k"), Hl::Property);
    EXPECT_EQ(kindOf(k, src, "sinosc"), Hl::Method);
    EXPECT_EQ(kindOf(k, src, "; note"), Hl::Comment);
}

TEST_F(HighlightTest, NumbersAndWordsSplitAsTheParserDoes) {
    std::string src = "1/12 2pi 10ms ping import sc.*\n1";
    std::vector<Hl> k = highlight(src);
    EXPECT_EQ(kindOf(k, src, "1/12"), Hl::Number);
    EXPECT_EQ(kindOf(k, src, "2pi"), Hl::Number);
    EXPECT_EQ(kindOf(k, src, "10ms"), Hl::Word);
    EXPECT_EQ(kindOf(k, src, "ping"), Hl::Word);
    EXPECT_EQ(kindOf(k, src, "import"), Hl::Keyword);
    EXPECT_EQ(kindOf(k, src, "sc"), Hl::Module);
    EXPECT_EQ(k.back(), Hl::Number);  // the import ended at the newline
}

TEST_F(HighlightTest, WordsByBinding) {
    th.mWorkspace = th.mWorkspace->putPure(V(getsym("hlBound")), V(1.));
    std::string src = "clear hlBound hlNowhere 10ms \\y [y] 3 = z z";
    std::vector<Hl> k = highlight(src, &th);
    EXPECT_EQ(kindOf(k, src, "clear"), Hl::Library);
    EXPECT_EQ(kindOf(k, src, "hlBound"), Hl::Bound);
    EXPECT_EQ(kindOf(k, src, "hlNowhere"), Hl::Unbound);
    EXPECT_EQ(kindOf(k, src, "10ms"), Hl::Unbound);
    EXPECT_EQ(kindOf(k, src, "y", 1), Hl::Bound);  // a parameter of the lambda
    EXPECT_EQ(kindOf(k, src, "z", 1), Hl::Bound);  // bound earlier in the line
}

TEST_F(HighlightTest, PreludeWordsAreLibraryUntilRebound) {
    th.mWorkspace = th.mWorkspace->putPure(V(getsym("hlLib")), V(1.))->putPure(V(getsym("hlRebound")), V(1.));
    th.mLibrary = th.mWorkspace;
    th.mWorkspace = th.mWorkspace->putPure(V(getsym("hlRebound")), V(2.))->putPure(V(getsym("hlLater")), V(3.));
    std::string src = "hlLib hlRebound hlLater";
    std::vector<Hl> k = highlight(src, &th);
    EXPECT_EQ(kindOf(k, src, "hlLib"), Hl::Library);
    EXPECT_EQ(kindOf(k, src, "hlRebound"), Hl::Bound);
    EXPECT_EQ(kindOf(k, src, "hlLater"), Hl::Bound);
}

TEST_F(HighlightTest, UGensApartFromFunctions) {
    V sc, sinosc;
    ASSERT_TRUE(vm.builtins->get(th, V(getsym("sc")), sc));
    ASSERT_TRUE(sc.o()->get(th, V(getsym("sinosc")), sinosc));
    th.mWorkspace = th.mWorkspace->putPure(V(getsym("hlOsc")), sinosc);
    th.mLibrary = th.mWorkspace;
    th.mWorkspace = th.mWorkspace->putPure(V(getsym("hlMyOsc")), sinosc);
    std::string src = "hlOsc sc.sinosc play hlMyOsc";
    std::vector<Hl> k = highlight(src, &th);
    EXPECT_EQ(kindOf(k, src, "hlOsc"), Hl::UGen);
    EXPECT_EQ(kindOf(k, src, "sinosc"), Hl::UGen);   // through the namespace
    EXPECT_EQ(kindOf(k, src, "play"), Hl::Library);
    EXPECT_EQ(kindOf(k, src, "hlMyOsc"), Hl::Bound);  // yours, though a UGen
}

TEST_F(HighlightTest, EmptyAndPartialInput) {
    EXPECT_TRUE(highlight("").empty());
    std::string src = "[1 \"open";
    EXPECT_EQ(highlight(src, &th).size(), src.size());
    EXPECT_FALSE(parsesCleanly(src));
}

// Every stac file and every example in the docs parses with no ERROR node.
TEST_F(HighlightTest, CorpusParsesCleanly) {
    fs::path root = SAPF_SOURCE_ROOT;
    std::vector<std::pair<std::string, std::string>> sources;
    for (const char* f : {"std.stac", "sapf.stac", "prelude.stac", "examples/sapf.stac", "examples.txt"})
        sources.push_back({f, read(root / f)});
    std::vector<fs::path> docs = {root / "docs" / "phrasebook.md"};
    for (auto& e : fs::directory_iterator(root / "docs" / "dictionary"))
        if (e.path().extension() == ".md") docs.push_back(e.path());
    std::regex block("```stac(?:-setup)?\\n([\\s\\S]*?)```");
    for (fs::path const& d : docs) {
        std::string text = read(d);
        int i = 0;
        for (std::sregex_iterator it(text.begin(), text.end(), block), end; it != end; ++it)
            sources.push_back({d.filename().string() + " block " + std::to_string(i++), (*it)[1]});
    }
    EXPECT_GT(sources.size(), 1000u);
    for (auto& [name, src] : sources) EXPECT_TRUE(parsesCleanly(src)) << name;
}
