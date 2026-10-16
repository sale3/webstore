// Jedinicni testovi alata za mutaciono testiranje (bez mock-a):
// pomocne funkcije za tekst, pravljenje mutanata, izvjestaji, fajlovi i plan.
// Testovi koji koriste MockCommandRunner su u CommandRunnerTest.cpp.

#include "TestHelpers.h"

#include <algorithm>
#include <csignal>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

using namespace mutation;

//========================================================================
// 1. Pomocne funkcije za tekst
//========================================================================

//----trim----

// tuple<ulaz, ocekivano>
class TrimTest : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(TrimTest, RemovesWhitespaceFromBothEnds) {
    // Arrange
    const auto& [input, expected] = GetParam();

    // Act
    const std::string result = trim(input);

    // Assert
    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(
    TrimCases,
    TrimTest,
    ::testing::Values(
        std::make_tuple("  if (x)  ", "if (x)"),
        std::make_tuple("\tx = 1;\r\n", "x = 1;"),
        std::make_tuple("bez razmaka", "bez razmaka"),
        std::make_tuple("   \t ", ""),
        std::make_tuple("", "")
    )
);

//----Krajevi redova----

TEST(SplitKeepEndingsTest, KeepsCrlfAndLfEndings) {
    // Act
    const auto lines = splitKeepEndings("a\r\nb\nc");

    // Assert
    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[0], "a\r\n");
    EXPECT_EQ(lines[1], "b\n");
    EXPECT_EQ(lines[2], "c");
}

TEST(SplitKeepEndingsTest, TextEndingWithNewlineHasNoEmptyLastLine) {
    // Act
    const auto lines = splitKeepEndings("a\nb\n");

    // Assert
    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[1], "b\n");
}

TEST(SplitKeepEndingsTest, EmptyTextGivesNoLines) {
    EXPECT_TRUE(splitKeepEndings("").empty());
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

// tuple<linija, kraj reda, linija bez kraja>
class LineEndingTest : public ::testing::TestWithParam<std::tuple<std::string, std::string, std::string>> {};

TEST_P(LineEndingTest, RecognizesAndRemovesEnding) {
    // Arrange
    const auto& [line, ending, withoutIt] = GetParam();

    // Act & Assert
    EXPECT_EQ(lineEnding(line), ending);
    EXPECT_EQ(withoutEnding(line), withoutIt);
}

INSTANTIATE_TEST_SUITE_P(
    Endings,
    LineEndingTest,
    ::testing::Values(
        std::make_tuple("a\r\n", "\r\n", "a"),
        std::make_tuple("a\n", "\n", "a"),
        std::make_tuple("a", "", "a"),
        std::make_tuple("", "", "")
    )
);

//----Ostale pomocne funkcije----

TEST(IndentOfTest, ReturnsLeadingSpacesAndTabs) {
    EXPECT_EQ(indentOf("    x = 1;"), "    ");
    EXPECT_EQ(indentOf("\t\tx"), "\t\t");
    EXPECT_EQ(indentOf("x"), "");
}

TEST(StartsWithTest, ChecksPrefix) {
    EXPECT_TRUE(startsWith("return x;", "return"));
    EXPECT_FALSE(startsWith("x = return;", "return"));
    EXPECT_FALSE(startsWith("ret", "return"));
}

//----Literali i komentari----

TEST(LiteralRangesTest, FindsStringAndCharLiterals) {
    // Act
    const auto ranges = literalRanges("a(\"x\", 'y');");

    // Assert
    ASSERT_EQ(ranges.size(), 2u);
    EXPECT_EQ(ranges[0], std::make_pair(std::size_t{2}, std::size_t{4}));
    EXPECT_EQ(ranges[1], std::make_pair(std::size_t{7}, std::size_t{9}));
}

TEST(LiteralRangesTest, SkipsEscapedQuoteInsideString) {
    // Act
    const auto ranges = literalRanges(R"(s = "a\"b";)");

    // Assert
    ASSERT_EQ(ranges.size(), 1u);
    EXPECT_EQ(ranges[0].first, 4u);
    EXPECT_EQ(ranges[0].second, 9u);
}

TEST(InsideLiteralTest, ChecksPositionAgainstRanges) {
    // Arrange
    const auto ranges = literalRanges("x = \"<=\";");

    // Act & Assert
    EXPECT_TRUE(insideLiteral(ranges, 5));
    EXPECT_FALSE(insideLiteral(ranges, 0));   // prije literala
    EXPECT_FALSE(insideLiteral(ranges, 8));   // poslije literala
}

// tuple<linija, kod bez komentara>
class CodePartTest : public ::testing::TestWithParam<std::tuple<std::string, std::string>> {};

TEST_P(CodePartTest, ReturnsCodeBeforeLineComment) {
    // Arrange
    const auto& [line, expected] = GetParam();

    // Act & Assert
    EXPECT_EQ(codePart(line), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Comments,
    CodePartTest,
    ::testing::Values(
        std::make_tuple("x = 1; // komentar", "x = 1; "),
        std::make_tuple("s = \"http://a\";", "s = \"http://a\";"),
        std::make_tuple("x = 1;", "x = 1;"),
        std::make_tuple("// cijeli komentar", "")
    )
);

//========================================================================
// 2. Pomocne funkcije za komande (bez pokretanja)
//========================================================================

//----Pomocne funkcije----

TEST(QuoteTest, WrapsPathInQuotesWithForwardSlashes) {
    EXPECT_EQ(quote(fs::path("build") / "debug"), "\"build/debug\"");
}

TEST(OnInterruptTest, SetsInterruptedFlag) {
    // Arrange
    g_interrupted = false;

    // Act
    onInterrupt(SIGINT);

    // Assert
    EXPECT_TRUE(g_interrupted);
    g_interrupted = false;
}

// tuple<status komande, da li je prekinuta sa Ctrl+C>
class InterruptedByCtrlCTest : public ::testing::TestWithParam<std::tuple<int, bool>> {};

TEST_P(InterruptedByCtrlCTest, RecognizesCtrlCStatus) {
    // Arrange
    const auto& [status, expected] = GetParam();

    // Act & Assert
    EXPECT_EQ(interruptedByCtrlC(status), expected);
}

#ifdef _WIN32
INSTANTIATE_TEST_SUITE_P(
    Statuses,
    InterruptedByCtrlCTest,
    ::testing::Values(
        std::make_tuple(static_cast<int>(0xC000013Au), true),  // STATUS_CONTROL_C_EXIT
        std::make_tuple(0, false),
        std::make_tuple(1, false)
    )
);
#else
INSTANTIATE_TEST_SUITE_P(
    Statuses,
    InterruptedByCtrlCTest,
    ::testing::Values(
        std::make_tuple(130 << 8, true),   // komanda zavrsila sa kodom 130
        std::make_tuple(SIGINT, true),     // komanda ubijena signalom SIGINT
        std::make_tuple(SIGTERM, false),   // ubijena drugim signalom
        std::make_tuple(1 << 8, false),    // zavrsila sa kodom 1
        std::make_tuple(0, false)          // uspjeh
    )
);
#endif

//========================================================================
// 3. Pravljenje mutanata iz jedne linije
//========================================================================

class MutateLineTest : public ::testing::Test {
protected:
    // Da li medju mutantima postoji mutant datog operatora sa datom linijom
    static bool hasMutant(const std::vector<LineMutation>& mutants,
                          const std::string& op, const std::string& newLine) {
        return std::any_of(mutants.begin(), mutants.end(), [&](const LineMutation& m) {
            return m.op == op && m.newLine == newLine;
        });
    }

    static bool hasOperator(const std::vector<LineMutation>& mutants, const std::string& op) {
        return std::any_of(mutants.begin(), mutants.end(),
                           [&](const LineMutation& m) { return m.op == op; });
    }
};

//----Svaki operator pravi ocekivanog mutanta----

// tuple<linija, operator, ocekivani mutant>
class MutateLineOperatorTest
    : public MutateLineTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string, std::string>> {};

TEST_P(MutateLineOperatorTest, ProducesExpectedMutant) {
    // Arrange
    const auto& [line, op, expected] = GetParam();

    // Act
    const auto mutants = mutateLine(line);

    // Assert
    EXPECT_TRUE(hasMutant(mutants, op, expected)) << "linija: " << line;
}

INSTANTIATE_TEST_SUITE_P(
    Operators,
    MutateLineOperatorTest,
    ::testing::Values(
        // ROR - relacioni operatori
        std::make_tuple("    if (q <= 0) {", "ROR", "    if (q < 0) {"),
        std::make_tuple("    if (q >= 0) {", "ROR", "    if (q > 0) {"),
        std::make_tuple("    return a == b;", "ROR", "    return a != b;"),
        std::make_tuple("    return a != b;", "ROR", "    return a == b;"),
        std::make_tuple("    if (a < b) {", "ROR", "    if (a <= b) {"),
        std::make_tuple("    if (a > b) {", "ROR", "    if (a >= b) {"),
        // LCR - logicki veznici
        std::make_tuple("    if (a && b) {", "LCR", "    if (a || b) {"),
        std::make_tuple("    if (a || b) {", "LCR", "    if (a && b) {"),
        // AOR - aritmeticki operatori
        std::make_tuple("    total += price;", "AOR", "    total -= price;"),
        std::make_tuple("    total -= price;", "AOR", "    total += price;"),
        std::make_tuple("    count++;", "AOR", "    count--;"),
        std::make_tuple("    x = a + b;", "AOR", "    x = a - b;"),
        std::make_tuple("    x = a - b;", "AOR", "    x = a + b;"),
        std::make_tuple("    x = a * b;", "AOR", "    x = a / b;"),
        std::make_tuple("    x = a / b;", "AOR", "    x = a * b;"),
        // UOD - uklanjanje negacije
        std::make_tuple("    if (!ok) {", "UOD", "    if (ok) {"),
        // BCR - logicke konstante
        std::make_tuple("    return true;", "BCR", "    return false;"),
        std::make_tuple("    return false;", "BCR", "    return true;"),
        // NCC - negacija cijelog uslova
        std::make_tuple("    if (stockReduced) {", "NCC", "    if (!(stockReduced)) {"),
        std::make_tuple("    else if (a) {", "NCC", "    else if (!(a)) {"),
        // CRP - konstanta N postaje N + 1
        std::make_tuple("    nextOrderId_(1000) {", "CRP", "    nextOrderId_(1001) {"),
        std::make_tuple("    d = 5.0;", "CRP", "    d = 6.0;"),
        // SDL - brisanje naredbe
        std::make_tuple("    items_.clear();", "SDL", "    /* MUTANT: items_.clear(); */"),
        std::make_tuple("    x = 5;", "SDL", "    /* MUTANT: x = 5; */")
    )
);

TEST_F(MutateLineTest, CommentAfterCodeIsKeptInMutant) {
    // Act
    const auto mutants = mutateLine("    x = a + b;  // zbir");

    // Assert
    EXPECT_TRUE(hasMutant(mutants, "AOR", "    x = a - b;  // zbir"));
}

TEST_F(MutateLineTest, EveryOccurrenceGetsItsOwnMutant) {
    // Act
    const auto mutants = mutateLine("    if (a <= b && c <= d) {");

    // Assert
    EXPECT_TRUE(hasMutant(mutants, "ROR", "    if (a < b && c <= d) {"));
    EXPECT_TRUE(hasMutant(mutants, "ROR", "    if (a <= b && c < d) {"));
}

//----Linije koje se ne mutiraju----

class MutateLineIgnoredTest
    : public MutateLineTest,
      public ::testing::WithParamInterface<std::string> {};

TEST_P(MutateLineIgnoredTest, ProducesNoMutants) {
    // Arrange
    const std::string line = GetParam();

    // Act & Assert
    EXPECT_TRUE(mutateLine(line).empty()) << "linija: " << line;
}

INSTANTIATE_TEST_SUITE_P(
    IgnoredLines,
    MutateLineIgnoredTest,
    ::testing::Values(
        "",
        "    ",
        "    // if (x <= 0)",
        "#include <vector>",
        "    /* x <= 0 */",
        "     * x <= 0",
        "}"
    )
);

//----Sta alat NE smije mutirati----

// tuple<linija, operator koji se ne smije pojaviti>
class MutateLineForbiddenTest
    : public MutateLineTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {};

TEST_P(MutateLineForbiddenTest, DoesNotProduceOperator) {
    // Arrange
    const auto& [line, op] = GetParam();

    // Act
    const auto mutants = mutateLine(line);

    // Assert
    EXPECT_FALSE(hasOperator(mutants, op)) << "linija: " << line;
}

INSTANTIATE_TEST_SUITE_P(
    ForbiddenMutations,
    MutateLineForbiddenTest,
    ::testing::Values(
        std::make_tuple("    log(\"a <= b\");", "ROR"),          // unutar stringa
        std::make_tuple("    c = '+';", "AOR"),                  // unutar char literala
        std::make_tuple("    x = 1;  // a <= b", "ROR"),          // u komentaru
        std::make_tuple("    std::vector<int> v;", "ROR"),        // sablon
        std::make_tuple("    it->second;", "ROR"),                // pokazivac
        std::make_tuple("    x <<= 1;", "ROR"),                   // <<= nije poredjenje
        std::make_tuple("    auto c = a <=> b;", "ROR"),          // <=> nije <=
        std::make_tuple("    x >>= 1;", "ROR"),                   // >>= nije poredjenje
        std::make_tuple("    x = -1;", "AOR"),                    // unarni minus
        std::make_tuple("    if (a != b) {", "UOD"),              // ! u !=
        std::make_tuple("    bool trueValue = x;", "BCR"),        // true u imenu
        std::make_tuple("    bool istrue = x;", "BCR"),           // true na kraju imena
        std::make_tuple("    return result;", "SDL"),             // return se ne brise
        std::make_tuple("    throw std::invalid_argument(\"x\");", "SDL"),
        std::make_tuple("    int x = 5;", "SDL"),                 // deklaracija se ne brise
        std::make_tuple("    utf8Helper();", "CRP"),              // broj u imenu
        std::make_tuple("    x = 1.0f;", "CRP"),                  // broj sa sufiksom
        std::make_tuple("    x = 10u;", "CRP"),                   // broj sa sufiksom
        std::make_tuple("    x = 12345678901;", "CRP"),           // prevelik broj
        std::make_tuple("    x = obj.5;", "CRP")                  // broj poslije tacke
    )
);

//----Granicni slucajevi: operator na pocetku ili na kraju linije----

INSTANTIATE_TEST_SUITE_P(
    LineEdges,
    MutateLineOperatorTest,
    ::testing::Values(
        std::make_tuple("==b", "ROR", "!=b"),              // operator na pocetku
        std::make_tuple("a <=", "ROR", "a <"),             // operator na kraju
        std::make_tuple("!ok", "UOD", "ok"),               // ! na pocetku
        std::make_tuple("x !", "UOD", "x "),               // ! na kraju
        std::make_tuple("true;", "BCR", "false;"),         // true na pocetku
        std::make_tuple("x = true", "BCR", "x = false"),   // true na kraju
        std::make_tuple("5;", "CRP", "6;"),                // broj na pocetku
        std::make_tuple("x = 5", "CRP", "x = 6"),          // broj na kraju
        std::make_tuple("x = 5.0", "CRP", "x = 6.0")       // decimalni broj na kraju
    )
);

INSTANTIATE_TEST_SUITE_P(
    LineEdges,
    MutateLineForbiddenTest,
    ::testing::Values(
        std::make_tuple("+ b", "AOR"),                      // nema razmaka ispred
        std::make_tuple("a +", "AOR"),                      // nema razmaka iza
        std::make_tuple("    if (a<!b) {", "UOD"),          // ! iza <
        std::make_tuple("    if (a>!b) {", "UOD"),          // ! iza >
        std::make_tuple("    bool true_flag = x;", "BCR"),  // _ je dio imena
        std::make_tuple("    x = utf8", "CRP"),             // ime na kraju linije
        std::make_tuple("    x = 5.", "CRP"),               // tacka bez decimala
        std::make_tuple("    x = 5.f;", "CRP"),             // tacka pa slovo
        std::make_tuple("    x = \"a <= b", "ROR")          // nezatvoren string
    )
);

//----Konkretan mutant koji ne smije nastati----

// tuple<linija, operator, mutant koji se ne smije pojaviti>
class MutateLineNotProducedTest
    : public MutateLineTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string, std::string>> {};

TEST_P(MutateLineNotProducedTest, DoesNotProduceMutant) {
    // Arrange
    const auto& [line, op, notExpected] = GetParam();

    // Act
    const auto mutants = mutateLine(line);

    // Assert
    EXPECT_FALSE(hasMutant(mutants, op, notExpected)) << "linija: " << line;
}

INSTANTIATE_TEST_SUITE_P(
    EqualsAfterComparison,
    MutateLineNotProducedTest,
    ::testing::Values(
        std::make_tuple("    if (a === b) {", "ROR", "    if (a =!= b) {"),
        std::make_tuple("    if (a !== b) {", "ROR", "    if (a !!= b) {"),
        std::make_tuple("    if (a <== b) {", "ROR", "    if (a <!= b) {"),
        std::make_tuple("    if (a >== b) {", "ROR", "    if (a >!= b) {")
    )
);

TEST_F(MutateLineTest, NegationAfterAssignmentIsNotRemoved) {
    // Act
    const auto mutants = mutateLine("    x =!ok;");

    // Assert
    EXPECT_FALSE(hasOperator(mutants, "UOD"));
}

TEST_F(MutateLineTest, DoubleNegationKeepsSecondNegation) {
    // Act
    const auto mutants = mutateLine("    if (!!ok) {");

    // Assert: uklanja se samo prvi !, drugi stoji iza ! i preskace se
    EXPECT_TRUE(hasMutant(mutants, "UOD", "    if (!ok) {"));
    EXPECT_EQ(std::count_if(mutants.begin(), mutants.end(),
                            [](const LineMutation& m) { return m.op == "UOD"; }),
              1);
}

//========================================================================
// 4. Skor i izvjestaji
//========================================================================

namespace {

Mutant makeMutant(int id, const std::string& file, int line, const std::string& op,
                  const std::string& status, const std::string& mutatedLine = "x < 0") {
    Mutant m;
    m.id = id;
    m.file = file;
    m.line = line;
    m.op = op;
    m.description = "opis";
    m.original = "x <= 0";
    m.mutatedLine = mutatedLine;
    m.status = status;
    return m;
}

bool contains(const std::string& text, const std::string& part) {
    return text.find(part) != std::string::npos;
}

}  // namespace

//----Status i skor----

// tuple<status, da li je ubijen>
class IsKilledTest : public ::testing::TestWithParam<std::tuple<std::string, bool>> {};

TEST_P(IsKilledTest, RecognizesKilledStatus) {
    // Arrange
    const auto& [status, expected] = GetParam();
    Mutant m;
    m.status = status;

    // Act & Assert
    EXPECT_EQ(isKilled(m), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Statuses,
    IsKilledTest,
    ::testing::Values(
        std::make_tuple("убијен", true),
        std::make_tuple("убијен (timeout)", true),
        std::make_tuple("преживио", false),
        std::make_tuple("не компајлира се", false),
        std::make_tuple("непокренут", false)
    )
);

class ScoreTest : public ::testing::Test {
protected:
    Mutant killed_ = makeMutant(1, "src/A.cpp", 1, "ROR", "убијен");
    Mutant timeout_ = makeMutant(2, "src/A.cpp", 2, "ROR", "убијен (timeout)");
    Mutant survived_ = makeMutant(3, "src/A.cpp", 3, "ROR", "преживио");
    Mutant notCompiled_ = makeMutant(4, "src/A.cpp", 4, "ROR", "не компајлира се");
};

TEST_F(ScoreTest, CountsKilledAndSurvived) {
    // Act
    const Score s = score({&killed_, &timeout_, &survived_});

    // Assert
    EXPECT_EQ(s.killed, 2);
    EXPECT_EQ(s.survived, 1);
    EXPECT_NEAR(s.percent, 66.67, 0.01);
}

TEST_F(ScoreTest, IgnoresMutantsThatDoNotCompile) {
    // Act
    const Score s = score({&killed_, &notCompiled_});

    // Assert
    EXPECT_EQ(s.killed, 1);
    EXPECT_EQ(s.survived, 0);
    EXPECT_DOUBLE_EQ(s.percent, 100.0);
}

TEST_F(ScoreTest, EmptyListGivesZero) {
    // Act
    const Score s = score({});

    // Assert
    EXPECT_EQ(s.killed, 0);
    EXPECT_DOUBLE_EQ(s.percent, 0.0);
}

//----Formatiranje----

// tuple<vrijednost, tekst>
class PercentTest : public ::testing::TestWithParam<std::tuple<double, std::string>> {};

TEST_P(PercentTest, FormatsWithOneDecimal) {
    // Arrange
    const auto& [value, expected] = GetParam();

    // Act & Assert
    EXPECT_EQ(percent(value), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Values,
    PercentTest,
    ::testing::Values(
        std::make_tuple(100.0, "100.0%"),
        std::make_tuple(95.25, "95.2%"),
        std::make_tuple(0.0, "0.0%")
    )
);

TEST(CellTest, EscapesPipeCharacter) {
    EXPECT_EQ(cell("a || b"), "a \\|\\| b");
}

TEST(CellTest, LeavesTextWithoutPipeUnchanged) {
    EXPECT_EQ(cell("x <= 0"), "x <= 0");
}

//----Izvjestaj automatskog rezima----

class CreateReportTest : public ::testing::Test {
protected:
    std::vector<Mutant> mutants_ = {
        makeMutant(1, "src/Cart.cpp", 11, "ROR", "убијен"),
        makeMutant(2, "src/Cart.cpp", 12, "AOR", "преживио", "a || b"),
        makeMutant(3, "src/Logger.cpp", 5, "ROR", "не компајлира се"),
    };
};

TEST_F(CreateReportTest, ContainsSummary) {
    // Act
    const std::string report = createReport(mutants_);

    // Assert
    EXPECT_TRUE(contains(report, "Укупно мутаната: **3**"));
    EXPECT_TRUE(contains(report, "Убијени: **1**"));
    EXPECT_TRUE(contains(report, "Преживјели: **1**"));
    EXPECT_TRUE(contains(report, "Не компајлирају се (не рачунају се): **1**"));
    EXPECT_TRUE(contains(report, "Мутациони скор: **50.0%**"));
}

TEST_F(CreateReportTest, ContainsTablesPerFileAndOperator) {
    // Act
    const std::string report = createReport(mutants_);

    // Assert
    EXPECT_TRUE(contains(report, "| src/Cart.cpp | 2 | 1 | 1 | 50.0% |"));
    EXPECT_TRUE(contains(report, "| src/Logger.cpp | 1 | 0 | 0 | 0.0% |"));
    EXPECT_TRUE(contains(report, "| ROR | 2 | 1 | 0 | 100.0% |"));
}

TEST_F(CreateReportTest, ListsSurvivingMutantWithEscapedPipes) {
    // Act
    const std::string report = createReport(mutants_);

    // Assert
    EXPECT_TRUE(contains(report, "## Преживјели мутанти"));
    EXPECT_TRUE(contains(report, "| 2 | src/Cart.cpp:12 | AOR (opis) | `x <= 0` | `a \\|\\| b` |"));
}

TEST_F(CreateReportTest, ShowsWhichTestKilledMutant) {
    // Arrange
    mutants_[0].killedBy = "CartTest.AddsProduct";

    // Act
    const std::string report = createReport(mutants_);

    // Assert
    EXPECT_TRUE(contains(report, "| убијен | CartTest.AddsProduct |"));
}

TEST_F(CreateReportTest, SaysWhenNoMutantSurvived) {
    // Arrange
    mutants_.erase(mutants_.begin() + 1);

    // Act
    const std::string report = createReport(mutants_);

    // Assert
    EXPECT_TRUE(contains(report, "Нема преживјелих мутаната."));
}

//----Izvjestaj planiranih mutacija----

class CreatePlanReportTest : public ::testing::Test {
protected:
    static Mutant planned(const std::string& id, const std::string& expected,
                          const std::string& status, const std::string& mutatedLine = "x < 0") {
        Mutant m = makeMutant(1, "src/Cart.cpp", 11, id, status, mutatedLine);
        m.expected = expected;
        return m;
    }
};

TEST_F(CreatePlanReportTest, CountsResultsMatchingExpectation) {
    // Arrange
    const std::vector<Mutant> mutants = {
        planned("M01", "killed", "убијен"),          // poklapa se
        planned("M02", "equivalent", "преживио"),    // poklapa se
        planned("M03", "killed", "преживио"),        // ne poklapa se
        planned("M04", "equivalent", "убијен"),      // ne poklapa se
    };

    // Act
    const std::string report = createPlanReport(mutants);

    // Assert
    EXPECT_TRUE(contains(report, "Планираних мутација: **4**"));
    EXPECT_TRUE(contains(report, "Резултат као што је очекивано: **2 / 4**"));
    EXPECT_TRUE(contains(report, "Скор без еквивалентних мутаната: **100.0%**"));
    EXPECT_TRUE(contains(report, "| убијен | убијен ✓ |"));
    EXPECT_TRUE(contains(report, "| еквивалентан | преживио ✓ |"));
    EXPECT_TRUE(contains(report, "| убијен | преживио ✗ |"));
    EXPECT_TRUE(contains(report, "| еквивалентан | убијен ✗ |"));
}

TEST_F(CreatePlanReportTest, ShowsDeletedLineAsDeleted) {
    // Arrange
    const std::vector<Mutant> mutants = {planned("M01", "killed", "убијен", "   ")};

    // Act
    const std::string report = createPlanReport(mutants);

    // Assert
    EXPECT_TRUE(contains(report, "*(обрисано)*"));
}

TEST_F(CreatePlanReportTest, ScoreWithoutEquivalentsIsZeroWhenAllAreEquivalent) {
    // Arrange
    const std::vector<Mutant> mutants = {planned("M01", "equivalent", "преживио")};

    // Act
    const std::string report = createPlanReport(mutants);

    // Assert
    EXPECT_TRUE(contains(report, "Скор без еквивалентних мутаната: **0.0%**"));
}

//========================================================================
// 5. Fajlovi, generisanje mutanata i plan
//========================================================================

//----Citanje i pisanje fajlova----

TEST_F(TempProjectTest, WriteAndReadKeepBytesUnchanged) {
    // Arrange
    const std::string content("a\r\nb\n\0d", 7);  // CRLF, LF i nula bajt

    // Act
    writeFile("src/x.txt", content);

    // Assert
    EXPECT_EQ(readFile("src/x.txt"), content);
}

TEST_F(TempProjectTest, ReadingMissingFileGivesEmptyText) {
    EXPECT_EQ(readFile("src/ne_postoji.cpp"), "");
}

//----Generisanje mutanata iz fajlova----

class GenerateMutantsTest : public TempProjectTest {
protected:
    void SetUp() override {
        TempProjectTest::SetUp();
        writeFile("src/A.cpp",
                  "/* blok komentar\n"
                  "   x <= 0 se ne mutira\n"
                  "*/\n"
                  "/* jednolinijski x <= 0 */\n"
                  "    if (q <= 0) {\n");
    }
};

TEST_F(GenerateMutantsTest, MutatesOnlyCodeOutsideComments) {
    // Act
    const auto mutants = generateMutants({root_ / "src" / "A.cpp"}, root_);

    // Assert
    ASSERT_FALSE(mutants.empty());
    for (const auto& m : mutants) {
        EXPECT_EQ(m.line, 5) << m.mutatedLine;
    }
}

TEST_F(GenerateMutantsTest, FillsMutantFields) {
    // Act
    const auto mutants = generateMutants({root_ / "src" / "A.cpp"}, root_);

    // Assert
    ASSERT_FALSE(mutants.empty());
    const Mutant& first = mutants.front();
    EXPECT_EQ(first.id, 1);
    EXPECT_EQ(first.file, "src/A.cpp");
    EXPECT_EQ(first.original, "if (q <= 0) {");
    EXPECT_EQ(first.op, "ROR");
    EXPECT_EQ(first.mutatedLine, "    if (q < 0) {");
    EXPECT_EQ(first.status, "непокренут");
}

TEST_F(GenerateMutantsTest, NumbersMutantsAcrossFiles) {
    // Arrange
    writeFile("src/B.cpp", "    count++;\n");

    // Act
    const auto mutants =
        generateMutants({root_ / "src" / "A.cpp", root_ / "src" / "B.cpp"}, root_);

    // Assert
    for (std::size_t i = 0; i < mutants.size(); ++i) {
        EXPECT_EQ(mutants[i].id, static_cast<int>(i) + 1);
    }
    EXPECT_EQ(mutants.back().file, "src/B.cpp");
}

//----Planirane mutacije----

class LoadPlanTest : public TempProjectTest {
protected:
    void SetUp() override {
        TempProjectTest::SetUp();
        writeFile("src/Cart.cpp",
                  "/* quantity <= 0 u komentaru\n"
                  "   quantity <= 0 i ovdje\n"
                  "*/\n"
                  "/* quantity <= 0 */\n"
                  "    if (quantity <= 0) {\r\n"
                  "    it->quantity += quantity;\n"
                  "    a = 1;\n"
                  "    b = 1;\n");
    }

    std::vector<Mutant> load(const std::string& plan) {
        writeFile("plan.txt", plan);
        return loadPlan(root_ / "plan.txt", root_);
    }
};

TEST_F(LoadPlanTest, ReadsMutationFromPlan) {
    // Act
    const auto mutants = load(
        "# komentar se preskace\n"
        "\n"
        "[M01] Kolicina 0 mora biti odbijena\n"
        "file:     src/Cart.cpp\n"
        "original: quantity <= 0\n"
        "mutant:   quantity < 0\n"
        "expected: killed\n");

    // Assert
    ASSERT_EQ(mutants.size(), 1u);
    const Mutant& m = mutants.front();
    EXPECT_EQ(m.id, 1);
    EXPECT_EQ(m.op, "M01");
    EXPECT_EQ(m.description, "Kolicina 0 mora biti odbijena");
    EXPECT_EQ(m.file, "src/Cart.cpp");
    EXPECT_EQ(m.line, 5);
    EXPECT_EQ(m.original, "if (quantity <= 0) {");
    EXPECT_EQ(m.mutatedLine, "    if (quantity < 0) {");
    EXPECT_EQ(m.expected, "killed");
}

TEST_F(LoadPlanTest, ExpectedDefaultsToKilled) {
    // Act
    const auto mutants = load(
        "[M01] Bez expected\n"
        "file: src/Cart.cpp\n"
        "original: += quantity\n"
        "mutant: = quantity\n");

    // Assert
    ASSERT_EQ(mutants.size(), 1u);
    EXPECT_EQ(mutants.front().expected, "killed");
    EXPECT_EQ(mutants.front().mutatedLine, "    it->quantity = quantity;");
}

TEST_F(LoadPlanTest, EmptyMutantDeletesCode) {
    // Act
    const auto mutants = load(
        "[M01] Brisanje\n"
        "file: src/Cart.cpp\n"
        "original: it->quantity += quantity;\n"
        "mutant:\n"
        "expected: equivalent\n");

    // Assert
    ASSERT_EQ(mutants.size(), 1u);
    EXPECT_EQ(mutants.front().mutatedLine, "    ");
    EXPECT_EQ(mutants.front().expected, "equivalent");
}

TEST_F(LoadPlanTest, ReadsSeveralMutations) {
    // Act
    const auto mutants = load(
        "[M01] Prva\nfile: src/Cart.cpp\noriginal: quantity <= 0\nmutant: quantity < 0\n"
        "[M02] Druga\nfile: src/Cart.cpp\noriginal: a = 1\nmutant: a = 2\n");

    // Assert
    ASSERT_EQ(mutants.size(), 2u);
    EXPECT_EQ(mutants[1].id, 2);
    EXPECT_EQ(mutants[1].op, "M02");
}

TEST_F(LoadPlanTest, HeaderWithoutClosingBracketUsesWholeLineAsId) {
    // Act
    const auto mutants = load(
        "[M01 bez zagrade\nfile: src/Cart.cpp\noriginal: a = 1\nmutant: a = 2\n");

    // Assert
    ASSERT_EQ(mutants.size(), 1u);
    EXPECT_EQ(mutants.front().op, "[M01 bez zagrade");
    EXPECT_EQ(mutants.front().description, "");
}

TEST_F(LoadPlanTest, EmptyPlanGivesNoMutations) {
    EXPECT_TRUE(load("# samo komentar\n").empty());
}

TEST_F(LoadPlanTest, ThrowsWhenPlanFileIsMissing) {
    EXPECT_THROW(loadPlan(root_ / "ne_postoji.txt", root_), std::runtime_error);
}

// tuple<plan, dio poruke o gresci>
class LoadPlanErrorTest
    : public LoadPlanTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {};

TEST_P(LoadPlanErrorTest, ReportsErrorInPlan) {
    // Arrange
    const auto& [plan, expectedMessage] = GetParam();

    // Act & Assert
    try {
        load(plan);
        FAIL() << "ocekivan je izuzetak";
    } catch (const std::runtime_error& e) {
        EXPECT_NE(std::string(e.what()).find(expectedMessage), std::string::npos) << e.what();
    }
}

INSTANTIATE_TEST_SUITE_P(
    PlanErrors,
    LoadPlanErrorTest,
    ::testing::Values(
        std::make_tuple("[M01] x\noriginal: a = 1\n",
                        "[M01] недостаје file или original"),
        std::make_tuple("[M01] x\nfile: src/Cart.cpp\n",
                        "[M01] недостаје file или original"),
        std::make_tuple("[M01] x\nfile: src/Cart.cpp\noriginal: a = 1\nexpected: maybe\n",
                        "[M01] expected мора бити killed или equivalent"),
        std::make_tuple("[M01] x\nfile: src/Nema.cpp\noriginal: a = 1\n",
                        "[M01] фајл не постоји: src/Nema.cpp"),
        std::make_tuple("[M01] x\nfile: src/Cart.cpp\noriginal: ne_postoji\n",
                        "појављује 0 пута"),
        std::make_tuple("[M01] x\nfile: src/Cart.cpp\noriginal: = 1\n",
                        "појављује 2 пута"),
        std::make_tuple("file: src/Cart.cpp\n",
                        "Неисправна линија у плану: file: src/Cart.cpp"),
        std::make_tuple("[M01] x\nlinija bez dvotacke\n",
                        "Неисправна линија у плану: linija bez dvotacke")
    )
);

TEST_F(LoadPlanTest, ReportsAllErrorsAtOnce) {
    // Act & Assert
    try {
        load("[M01] x\nfile: src/Cart.cpp\n"
             "[M02] y\nfile: src/Nema.cpp\noriginal: a\n");
        FAIL() << "ocekivan je izuzetak";
    } catch (const std::runtime_error& e) {
        const std::string message = e.what();
        EXPECT_NE(message.find("[M01]"), std::string::npos);
        EXPECT_NE(message.find("[M02]"), std::string::npos);
    }
}