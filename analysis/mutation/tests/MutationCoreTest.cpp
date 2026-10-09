// Јединични тестови за алат за мутационо тестирање.
// Тестирају се функције које раде само са текстом, без фајлова и cmake-а.

#include "mutation_core.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>

using namespace mutation;

namespace {

// Да ли међу мутантима постоји мутант датог оператора са датом линијом
bool hasMutant(const std::vector<LineMutation>& mutants,
               const std::string& op, const std::string& newLine) {
    return std::any_of(mutants.begin(), mutants.end(), [&](const LineMutation& m) {
        return m.op == op && m.newLine == newLine;
    });
}

bool hasOperator(const std::vector<LineMutation>& mutants, const std::string& op) {
    return std::any_of(mutants.begin(), mutants.end(),
                       [&](const LineMutation& m) { return m.op == op; });
}

}  // namespace

// ---------- Помоћне функције за текст ----------

TEST(TrimTest, RemovesWhitespaceFromBothEnds) {
    EXPECT_EQ(trim("  \tif (x)  \r\n"), "if (x)");
}

TEST(TrimTest, ReturnsEmptyForWhitespaceOnly) {
    EXPECT_EQ(trim("   \t"), "");
}

TEST(SplitKeepEndingsTest, KeepsCrlfAndLfEndings) {
    // Act
    const auto lines = splitKeepEndings("a\r\nb\nc");

    // Assert
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "a\r\n");
    EXPECT_EQ(lines[1], "b\n");
    EXPECT_EQ(lines[2], "c");
}

TEST(SplitKeepEndingsTest, JoiningLinesGivesOriginalText) {
    // Arrange
    const std::string text = "x = 1;\r\ny = 2;\nz = 3;";

    // Act
    std::string joined;
    for (const auto& line : splitKeepEndings(text)) joined += line;

    // Assert
    EXPECT_EQ(joined, text);
}

TEST(LineEndingTest, RecognizesAllEndings) {
    EXPECT_EQ(lineEnding("a\r\n"), "\r\n");
    EXPECT_EQ(lineEnding("a\n"), "\n");
    EXPECT_EQ(lineEnding("a"), "");
}

TEST(CodePartTest, CutsLineComment) {
    EXPECT_EQ(codePart("x = 1; // komentar"), "x = 1; ");
}

TEST(CodePartTest, KeepsDoubleSlashInsideString) {
    EXPECT_EQ(codePart("s = \"http://a\";"), "s = \"http://a\";");
}

// ---------- Мутациони оператори ----------

// tuple<линија кода, оператор, очекивани мутант>
class MutateLineOperatorTest
    : public ::testing::TestWithParam<std::tuple<std::string, std::string, std::string>> {};

TEST_P(MutateLineOperatorTest, ProducesExpectedMutant) {
    // Arrange
    const auto& [line, op, expected] = GetParam();

    // Act
    const auto mutants = mutateLine(line);

    // Assert
    EXPECT_TRUE(hasMutant(mutants, op, expected)) << "линија: " << line;
}

INSTANTIATE_TEST_SUITE_P(
    Operators,
    MutateLineOperatorTest,
    ::testing::Values(
        std::make_tuple("    if (quantity <= 0) {", "ROR", "    if (quantity < 0) {"),
        std::make_tuple("    return a == b;", "ROR", "    return a != b;"),
        std::make_tuple("    if (a && b) {", "LCR", "    if (a || b) {"),
        std::make_tuple("    total += price;", "AOR", "    total -= price;"),
        std::make_tuple("    x = a * b;", "AOR", "    x = a / b;"),
        std::make_tuple("    if (!ok) {", "UOD", "    if (ok) {"),
        std::make_tuple("    return true;", "BCR", "    return false;"),
        std::make_tuple("    if (stockReduced) {", "NCC", "    if (!(stockReduced)) {"),
        std::make_tuple("    nextOrderId_(1000) {", "CRP", "    nextOrderId_(1001) {"),
        std::make_tuple("    d = 5.0;", "CRP", "    d = 6.0;"),
        std::make_tuple("    items_.clear();", "SDL", "    /* MUTANT: items_.clear(); */")
    )
);

// ---------- Шта алат НЕ смије мутирати ----------

TEST(MutateLineTest, IgnoresEmptyLine) {
    EXPECT_TRUE(mutateLine("   ").empty());
}

TEST(MutateLineTest, IgnoresLineComments) {
    EXPECT_TRUE(mutateLine("    // if (x <= 0)").empty());
}

TEST(MutateLineTest, IgnoresPreprocessorLines) {
    EXPECT_TRUE(mutateLine("#include <vector>").empty());
}

TEST(MutateLineTest, DoesNotMutateInsideStrings) {
    // Act
    const auto mutants = mutateLine("    log(\"a <= b\");");

    // Assert
    EXPECT_FALSE(hasOperator(mutants, "ROR"));
}

TEST(MutateLineTest, DoesNotMutateTrailingComment) {
    // Act
    const auto mutants = mutateLine("    x = 1;  // a <= b");

    // Assert
    EXPECT_FALSE(hasOperator(mutants, "ROR"));
}

TEST(MutateLineTest, DoesNotTouchTemplateBrackets) {
    // Act
    const auto mutants = mutateLine("    std::vector<int> v;");

    // Assert
    EXPECT_FALSE(hasOperator(mutants, "ROR"));
}

TEST(MutateLineTest, DoesNotTouchPointerArrow) {
    // Act
    const auto mutants = mutateLine("    it->second = 5;");

    // Assert
    EXPECT_FALSE(hasMutant(mutants, "AOR", "    it+>second = 5;"));
    EXPECT_FALSE(hasOperator(mutants, "ROR"));
}

TEST(MutateLineTest, DoesNotDeleteReturnStatement) {
    EXPECT_FALSE(hasOperator(mutateLine("    return result;"), "SDL"));
}

TEST(MutateLineTest, DoesNotMutateNumbersInsideIdentifiers) {
    // Act
    const auto mutants = mutateLine("    utf8Helper();");

    // Assert
    EXPECT_FALSE(hasOperator(mutants, "CRP"));
}

// ---------- Скор и извјештај ----------

TEST(ScoreTest, CountsKilledAndSurvived) {
    // Arrange
    Mutant killed, timeout, survived;
    killed.status = "убијен";
    timeout.status = "убијен (timeout)";
    survived.status = "преживио";

    // Act
    const Score s = score({&killed, &timeout, &survived});

    // Assert
    EXPECT_EQ(s.killed, 2);
    EXPECT_EQ(s.survived, 1);
    EXPECT_NEAR(s.percent, 66.67, 0.01);
}

TEST(ScoreTest, IgnoresMutantsThatDoNotCompile) {
    // Arrange
    Mutant killed, notCompiled;
    killed.status = "убијен";
    notCompiled.status = "не компајлира се";

    // Act
    const Score s = score({&killed, &notCompiled});

    // Assert
    EXPECT_EQ(s.killed, 1);
    EXPECT_EQ(s.survived, 0);
    EXPECT_DOUBLE_EQ(s.percent, 100.0);
}

TEST(ScoreTest, EmptyListGivesZero) {
    EXPECT_DOUBLE_EQ(score({}).percent, 0.0);
}

TEST(CellTest, EscapesPipeCharacter) {
    EXPECT_EQ(cell("a || b"), "a \\|\\| b");
}

TEST(PercentTest, FormatsWithOneDecimal) {
    EXPECT_EQ(percent(95.25), "95.2%");
    EXPECT_EQ(percent(100.0), "100.0%");
}
