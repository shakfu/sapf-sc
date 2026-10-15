//    sapf-sc - sapf interpreter for SuperCollider
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

#include <gtest/gtest.h>
#include "test_common.hpp"
#include "Completion.hpp"
#include <algorithm>

using Strings = std::vector<std::string>;

class CompletionTest : public SapfTestBase {
protected:
    Thread th;

    bool offers(const std::string& context, const std::string& word) {
        Strings all = completions(th, context);
        return std::find(all.begin(), all.end(), word) != all.end();
    }
};

TEST_F(CompletionTest, ContextIsTheNameAtTheEnd) {
    EXPECT_EQ(completionContext("440 ug.sino"), "ug.sino");
    EXPECT_EQ(completionContext("[1 2] sinosc.k"), "sinosc.k");
    EXPECT_EQ(completionContext("(a b"), "b");
    EXPECT_EQ(completionContext("{:freq 300} 'pin"), "pin");
    EXPECT_EQ(completionContext("5 =x"), "x");
    EXPECT_EQ(completionContext("\\fr"), "fr");
    EXPECT_EQ(completionContext("1 2 "), "");
}

TEST_F(CompletionTest, NamespaceWords) {
    EXPECT_EQ(completions(th, "env.p"), (Strings{"env.perc"}));
    EXPECT_TRUE(offers("ug.sinos", "ug.sinosc"));
    EXPECT_TRUE(offers("fft.pv_mag", "fft.pv_magfreeze"));
}

TEST_F(CompletionTest, PlainPrefixOffersNamespacedWordsByCategory) {
    // fft holds it, so ug's index entry is not offered too
    EXPECT_EQ(completions(th, "pv_magf"), (Strings{"fft.pv_magfreeze"}));
    EXPECT_TRUE(offers("pbin", "pbind"));
    EXPECT_TRUE(offers("dse", "demand.dseq"));
}

TEST_F(CompletionTest, MethodsAfterAValue) {
    EXPECT_TRUE(offers("sinosc.k", "sinosc.kr"));
    EXPECT_EQ(completions(th, ".do"), (Strings{".done"}));
    EXPECT_EQ(completions(th, "n.rel"), (Strings{"n.release", "n.releaseIn"}));
}

TEST_F(CompletionTest, UserBindings) {
    P<Fun> fun;
    ASSERT_TRUE(th.compile("5 = myvariable", fun, true));
    fun->apply(th);
    EXPECT_EQ(completions(th, "myvar"), (Strings{"myvariable"}));
}

TEST_F(CompletionTest, StackEffect) {
    EXPECT_EQ(stackEffect(th, "+"), "(x y --> z)");
    EXPECT_EQ(stackEffect(th, "ug.sinosc"), "(freq --> out)");
    EXPECT_EQ(stackEffect(th, "env.perc"), "(attackTime releaseTime --> env)");
    EXPECT_EQ(stackEffect(th, "nosuchword"), "");
    EXPECT_EQ(stackEffect(th, "nosuchns.word"), "");
}

TEST_F(CompletionTest, NamespacedWords) {
    EXPECT_EQ(namespacedWords(th, "dseq"), (Strings{"demand.dseq"}));
    EXPECT_EQ(namespacedWords(th, "sinosc"), (Strings{"ug.sinosc"}));
    EXPECT_EQ(namespacedWords(th, "nosuchword"), Strings{});
}
