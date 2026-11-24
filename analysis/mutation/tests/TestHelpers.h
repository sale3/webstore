#pragma once

#include "mutation_core.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <random>
#include <string>

namespace fs = std::filesystem;

// Mock za pokretanje komandi (cmake, ctest), umjesto std::system
class MockCommandRunner : public mutation::CommandRunner {
public:
    MOCK_METHOD(int, run, (const std::string& command, const fs::path& log), (override));
};

// Status koji sistem vrati kad korisnik prekine komandu sa Ctrl+C
inline int ctrlCStatus() {
#ifdef _WIN32
    return static_cast<int>(0xC000013Au);
#else
    return 130 << 8;  // komanda je zavrsila sa izlaznim kodom 130
#endif
}

// Fixture sa privremenim "projektom" na disku: root/src, root/build/debug ...
// Svaki test dobija svoj prazan folder, koji se na kraju brise.
class TempProjectTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::random_device random;
        root_ = fs::temp_directory_path() /
                ("mutation_tool_test_" + std::to_string(random()) + std::to_string(random()));
        fs::create_directories(root_ / "src");
        buildDir_ = root_ / "build" / "debug";
        fs::create_directories(buildDir_);
        mutation::g_interrupted = false;
    }

    void TearDown() override {
        mutation::g_interrupted = false;
        std::error_code ignored;
        fs::remove_all(root_, ignored);
    }

    // Upise fajl relativno u odnosu na root projekta
    void writeFile(const std::string& relativePath, const std::string& content) const {
        const fs::path path = root_ / relativePath;
        fs::create_directories(path.parent_path());
        mutation::writeBytes(path, content);
    }

    std::string readFile(const std::string& relativePath) const {
        return mutation::readBytes(root_ / relativePath);
    }

    fs::path root_;
    fs::path buildDir_;
};
