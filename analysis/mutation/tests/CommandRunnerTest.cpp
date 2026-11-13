// Jedinicni testovi dijelova alata koji pokrecu komande (cmake, ctest).
// Komande se ne pokrecu stvarno: umjesto njih se koristi MockCommandRunner.

#include "TestHelpers.h"

#include <stdexcept>
#include <string>
#include <tuple>

using namespace mutation;
using ::testing::_;
using ::testing::HasSubstr;
using ::testing::Not;
using ::testing::Return;
using ::testing::StrictMock;

//----Komande preko mock-a----

class RunnerTest : public TempProjectTest {
protected:
    StrictMock<MockCommandRunner> runner_;

    // Akcija za mock: upise dati tekst u log fajl koji je komanda dobila
    static auto writesLog(const std::string& content) {
        return [content](const std::string&, const fs::path& log) {
            writeBytes(log, content);
            return 1;
        };
    }
};

TEST_F(RunnerTest, RunCommandReturnsTrueOnZeroStatus) {
    // Arrange
    EXPECT_CALL(runner_, run("cmd", _)).WillOnce(Return(0));

    // Act & Assert
    EXPECT_TRUE(runCommand(runner_, "cmd", buildDir_ / "x.log"));
    EXPECT_FALSE(g_interrupted);
}

TEST_F(RunnerTest, RunCommandReturnsFalseOnNonZeroStatus) {
    // Arrange
    EXPECT_CALL(runner_, run("cmd", _)).WillOnce(Return(1));

    // Act & Assert
    EXPECT_FALSE(runCommand(runner_, "cmd", buildDir_ / "x.log"));
    EXPECT_FALSE(g_interrupted);
}

TEST_F(RunnerTest, RunCommandSetsInterruptedOnCtrlC) {
    // Arrange
    EXPECT_CALL(runner_, run("cmd", _)).WillOnce(Return(ctrlCStatus()));

    // Act
    const bool ok = runCommand(runner_, "cmd", buildDir_ / "x.log");

    // Assert
    EXPECT_FALSE(ok);
    EXPECT_TRUE(g_interrupted);
}

TEST_F(RunnerTest, BuildCompilesOnlyTestTarget) {
    // Arrange
    EXPECT_CALL(runner_, run(HasSubstr("--target order_system_tests"),
                             buildDir_ / "mutation_build.log"))
        .WillOnce(Return(0));

    // Act & Assert
    EXPECT_TRUE(build(runner_, buildDir_));
}

TEST_F(RunnerTest, RunTestsReturnsPassedAndSkipsToolTests) {
    // Arrange
    EXPECT_CALL(runner_, run(HasSubstr("-E \"^MutationTool\\.\""), _)).WillOnce(Return(0));

    // Act & Assert
    EXPECT_EQ(runTests(runner_, buildDir_), "passed");
}

// tuple<sadrzaj loga, ocekivani rezultat>
class RunTestsFailureTest
    : public RunnerTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {};

TEST_P(RunTestsFailureTest, RecognizesFailureKind) {
    // Arrange
    const auto& [log, expected] = GetParam();
    EXPECT_CALL(runner_, run(HasSubstr("ctest"), _)).WillOnce(writesLog(log));

    // Act & Assert
    EXPECT_EQ(runTests(runner_, buildDir_), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Failures,
    RunTestsFailureTest,
    ::testing::Values(
        std::make_tuple("1/5 Test #1: CartTest.X ...***Timeout 30.01 sec", "timeout"),
        std::make_tuple("1/5 Test #1: CartTest.X ...***Failed", "failed")
    )
);

//----Ime prvog testa koji je pao----

// tuple<sadrzaj loga, ocekivano ime>
class FirstFailedTestTest
    : public TempProjectTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string>> {};

TEST_P(FirstFailedTestTest, ExtractsTestName) {
    // Arrange
    const auto& [log, expected] = GetParam();
    writeFile("build/debug/mutation_test.log", log);

    // Act & Assert
    EXPECT_EQ(firstFailedTest(buildDir_), expected);
}

INSTANTIATE_TEST_SUITE_P(
    Logs,
    FirstFailedTestTest,
    ::testing::Values(
        std::make_tuple("The following tests FAILED:\n\t 45 - CartTest.AddsProduct (Failed)\n",
                        "CartTest.AddsProduct"),
        std::make_tuple("100% tests passed", ""),
        std::make_tuple("The following tests FAILED:\n", ""),
        std::make_tuple("The following tests FAILED:\n 45 - CartTest.AddsProduct", "")
    )
);

//----Konfigurisanje build foldera----

TEST_F(RunnerTest, EnsureConfiguredDoesNothingWhenAlreadyConfigured) {
    // Arrange: StrictMock obara test ako se pozove ijedna komanda
    writeFile("build/debug/CMakeCache.txt", "");

    // Act & Assert
    EXPECT_NO_THROW(ensureConfigured(runner_, buildDir_, root_));
}

TEST_F(RunnerTest, EnsureConfiguredUsesPresetForDebugFolder) {
    // Arrange
    writeFile("CMakePresets.json", "{}");
    EXPECT_CALL(runner_, run("cmake --preset debug", _)).WillOnce(Return(0));

    // Act & Assert
    EXPECT_NO_THROW(ensureConfigured(runner_, buildDir_, root_));
}

TEST_F(RunnerTest, EnsureConfiguredUsesSourceAndBuildDirWithoutPresets) {
    // Arrange
    EXPECT_CALL(runner_, run(HasSubstr("-DBUILD_TESTS=ON"), _)).WillOnce(Return(0));

    // Act & Assert
    EXPECT_NO_THROW(ensureConfigured(runner_, buildDir_, root_));
}

TEST_F(RunnerTest, EnsureConfiguredIgnoresPresetForOtherFolder) {
    // Arrange
    writeFile("CMakePresets.json", "{}");
    const fs::path otherDir = root_ / "build" / "other";
    EXPECT_CALL(runner_, run(Not(HasSubstr("--preset")), _)).WillOnce(Return(0));

    // Act & Assert
    EXPECT_NO_THROW(ensureConfigured(runner_, otherDir, root_));
    EXPECT_TRUE(fs::exists(otherDir));
}

TEST_F(RunnerTest, EnsureConfiguredThrowsWhenCMakeFails) {
    // Arrange
    EXPECT_CALL(runner_, run(_, _)).WillOnce(Return(1));

    // Act & Assert
    EXPECT_THROW(ensureConfigured(runner_, buildDir_, root_), std::runtime_error);
}

//----Testiranje jednog mutanta----

class TestMutantTest : public RunnerTest {
protected:
    // CRLF namjerno, da se provjeri da se fajl vraca bajt po bajt
    static constexpr const char* kOriginal = "int f(int q) {\r\n    if (q <= 0) {\r\n}\r\n";

    void SetUp() override {
        RunnerTest::SetUp();
        writeFile("src/Foo.cpp", kOriginal);
        mutant_.file = "src/Foo.cpp";
        mutant_.line = 2;
        mutant_.mutatedLine = "    if (q < 0) {";
    }

    // Akcija za build: provjeri da je mutant upisan u fajl dok se gradi
    auto buildSeesMutant(int status) {
        return [this, status](const std::string&, const fs::path&) {
            EXPECT_EQ(readFile("src/Foo.cpp"),
                      "int f(int q) {\r\n    if (q < 0) {\r\n}\r\n");
            return status;
        };
    }

    Mutant mutant_;
};

TEST_F(TestMutantTest, MutantThatDoesNotCompileIsMarked) {
    // Arrange
    EXPECT_CALL(runner_, run(HasSubstr("--build"), _)).WillOnce(buildSeesMutant(1));

    // Act
    testMutant(mutant_, root_, buildDir_, runner_);

    // Assert
    EXPECT_EQ(mutant_.status, "не компајлира се");
    EXPECT_EQ(readFile("src/Foo.cpp"), kOriginal);
}

TEST_F(TestMutantTest, MutantSurvivesWhenTestsPass) {
    // Arrange
    EXPECT_CALL(runner_, run(HasSubstr("--build"), _)).WillOnce(buildSeesMutant(0));
    EXPECT_CALL(runner_, run(HasSubstr("ctest"), _)).WillOnce(Return(0));

    // Act
    testMutant(mutant_, root_, buildDir_, runner_);

    // Assert
    EXPECT_EQ(mutant_.status, "преживио");
    EXPECT_EQ(mutant_.killedBy, "");
    EXPECT_EQ(readFile("src/Foo.cpp"), kOriginal);
}

// tuple<log testova, ocekivani status, ocekivani test koji je ubio mutanta>
class TestMutantKilledTest
    : public TestMutantTest,
      public ::testing::WithParamInterface<std::tuple<std::string, std::string, std::string>> {};

TEST_P(TestMutantKilledTest, MutantIsKilledWhenTestsFail) {
    // Arrange
    const auto& [log, expectedStatus, expectedKilledBy] = GetParam();
    EXPECT_CALL(runner_, run(HasSubstr("--build"), _)).WillOnce(buildSeesMutant(0));
    EXPECT_CALL(runner_, run(HasSubstr("ctest"), _)).WillOnce(writesLog(log));

    // Act
    testMutant(mutant_, root_, buildDir_, runner_);

    // Assert
    EXPECT_EQ(mutant_.status, expectedStatus);
    EXPECT_EQ(mutant_.killedBy, expectedKilledBy);
    EXPECT_EQ(readFile("src/Foo.cpp"), kOriginal);
}

INSTANTIATE_TEST_SUITE_P(
    KilledMutants,
    TestMutantKilledTest,
    ::testing::Values(
        std::make_tuple("The following tests FAILED:\n 7 - CartTest.X (Failed)\n",
                        "убијен", "CartTest.X"),
        std::make_tuple("***Timeout\nThe following tests FAILED:\n 7 - CartTest.Y (Timeout)\n",
                        "убијен (timeout)", "CartTest.Y")
    )
);

TEST_F(TestMutantTest, InterruptedRunIsMarkedAsNotRun) {
    // Arrange
    EXPECT_CALL(runner_, run(HasSubstr("--build"), _)).WillOnce(Return(ctrlCStatus()));

    // Act
    testMutant(mutant_, root_, buildDir_, runner_);

    // Assert
    EXPECT_EQ(mutant_.status, "непокренут");
    EXPECT_EQ(readFile("src/Foo.cpp"), kOriginal);
}
