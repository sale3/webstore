// Јавни дио алата за мутационо тестирање: структуре и функције
// које користе и mutation_tester (main) и тестови алата.
#pragma once

#include <atomic>
#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace mutation {

namespace fs = std::filesystem;

// Један мутант: гдје је направљен, како изгледа и како је прошао.
struct Mutant {
    int id = 0;
    std::string file;          // путања релативно у односу на root пројекта
    int line = 0;              // број линије (од 1)
    std::string op;            // скраћеница оператора (ROR, AOR, ...)
    std::string description;   // опис измјене
    std::string original;      // оригинална линија (без увлачења)
    std::string mutatedLine;   // цијела измијењена линија (са увлачењем)
    std::string status = "непокренут";
    std::string killedBy;      // први тест који је пао
    std::string expected;      // само за планиране мутације: killed или equivalent
};

// Једна измјена коју mutateLine направи у линији кода.
struct LineMutation {
    std::string op;
    std::string description;
    std::string newLine;
};

struct Score {
    int killed = 0;
    int survived = 0;
    double percent = 0.0;
};

// Постаје true кад корисник притисне Ctrl+C.
extern std::atomic<bool> g_interrupted;
void onInterrupt(int);

// ---------- Помоћне функције за текст и фајлове ----------
std::string readBytes(const fs::path& path);
void writeBytes(const fs::path& path, const std::string& content);
std::vector<std::string> splitKeepEndings(const std::string& text);
std::string lineEnding(const std::string& line);
std::string withoutEnding(const std::string& line);
std::string trim(const std::string& s);
std::string indentOf(const std::string& line);
bool startsWith(const std::string& s, const std::string& prefix);
std::vector<std::pair<std::size_t, std::size_t>> literalRanges(const std::string& code);
bool insideLiteral(const std::vector<std::pair<std::size_t, std::size_t>>& ranges,
                   std::size_t pos);
std::string codePart(const std::string& line);

// ---------- Мутациони оператори ----------
std::vector<LineMutation> mutateLine(const std::string& line);
std::vector<Mutant> generateMutants(const std::vector<fs::path>& files, const fs::path& root);

// ---------- Компајлирање и покретање тестова ----------

// Покретање спољних команди (cmake, ctest) издвојено је у интерфејс,
// да би се у тестовима могло замијенити mock објектом.
class CommandRunner {
public:
    virtual ~CommandRunner() = default;
    // Покреће команду, излаз пише у log. Враћа статус који врати систем (0 = успјех).
    virtual int run(const std::string& command, const fs::path& log) = 0;
};

// Права имплементација: команду покреће преко std::system.
class SystemCommandRunner : public CommandRunner {
public:
    int run(const std::string& command, const fs::path& log) override;
};

std::string quote(const fs::path& path);
bool interruptedByCtrlC(int status);
bool runCommand(CommandRunner& runner, const std::string& command, const fs::path& log);
bool build(CommandRunner& runner, const fs::path& buildDir);
std::string runTests(CommandRunner& runner, const fs::path& buildDir);
std::string firstFailedTest(const fs::path& buildDir);
void ensureConfigured(CommandRunner& runner, const fs::path& buildDir, const fs::path& root);
void testMutant(Mutant& mutant, const fs::path& root, const fs::path& buildDir,
                CommandRunner& runner);

// ---------- Извјештај ----------
bool isKilled(const Mutant& m);
Score score(const std::vector<const Mutant*>& mutants);
std::string percent(double value);
std::string cell(const std::string& text);
std::string createReport(const std::vector<Mutant>& mutants);

// ---------- Планиране мутације (--plan) ----------
// Баца std::runtime_error ако фајл не постоји или план има грешке.
std::vector<Mutant> loadPlan(const fs::path& planPath, const fs::path& root);
std::string createPlanReport(const std::vector<Mutant>& mutants);

}  // namespace mutation