#include <gtest/gtest.h>
#include "auxiliar_lexer.hpp"

TEST(Infraestrutura, ScannerEhAlcancavelPelosTestes) {
    auto toks = tokenizar("se");
    ASSERT_EQ(toks.size(), 1u);
    EXPECT_EQ(toks[0].codigo, KW_SE);
}