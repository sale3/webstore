// Логика алата за мутационо тестирање (све осим main-a)
// Издвојена у засебан фајл да би се могла тестирати Google Testом.

#include "mutation_core.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace mutation {

std::atomic<bool> g_interrupted{false};

void onInterrupt(int) { g_interrupted = true; }

// Помоћне функције за текст и фајлове

std::string readBytes(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

void writeBytes(const fs::path& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out << content;
}

// дијели текст на линије и чува крај сваке линије ("\r\n", "\n" или "").
std::vector<std::string> splitKeepEndings(const std::string& text) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) {
            lines.push_back(text.substr(start));
            break;
        }
        lines.push_back(text.substr(start, end - start + 1));
        start = end + 1;
    }
    return lines;
}
// враћа крај линије
std::string lineEnding(const std::string& line) {
    if (line.size() >= 2 && line.compare(line.size() - 2, 2, "\r\n") == 0) return "\r\n";
    if (!line.empty() && line.back() == '\n') return "\n";
    return "";
}
//линија без краја
std::string withoutEnding(const std::string& line) {
    return line.substr(0, line.size() - lineEnding(line).size());
}
//уклања размаке, табове, крајеве реда са оба краја
std::string trim(const std::string& s) {
    std::size_t begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}
//враћа увлачење са почетка линије
std::string indentOf(const std::string& line) {
    return line.substr(0, line.find_first_not_of(" \t"));
}
//враћа позиције стрингова - нпр одд 4 до 11 позиције је стринг
bool startsWith(const std::string& s, const std::string& prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

bool isWordChar(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool isSpace(char c) {
    return std::isspace(static_cast<unsigned char>(c)) != 0;
}

// опсези стринг и char литерала ("..." и '...') у линији.
std::vector<std::pair<std::size_t, std::size_t>> literalRanges(const std::string& code) {
    std::vector<std::pair<std::size_t, std::size_t>> ranges;
    for (std::size_t i = 0; i < code.size(); ++i) {
        const char quote = code[i];
        if (quote != '"' && quote != '\'') continue;
        std::size_t j = i + 1;
        while (j < code.size() && code[j] != quote) {
            if (code[j] == '\\') ++j;
            ++j;
        }
        ranges.emplace_back(i, j);
        i = j;
    }
    return ranges;
}
//каже да је позиција унутар литерала
bool insideLiteral(const std::vector<std::pair<std::size_t, std::size_t>>& ranges,
                   std::size_t pos) {
    for (const auto& [begin, end] : ranges) {
        if (pos >= begin && pos <= end) return true;
    }
    return false;
}

// дио линије прије // коментара (који није унутар стринга) нпр "x = a + b;  // zbir"      →  "x = a + b;  "
std::string codePart(const std::string& line) {
    const auto ranges = literalRanges(line);
    for (std::size_t i = 0; i + 1 < line.size(); ++i) {
        if (line[i] == '/' && line[i + 1] == '/' && !insideLiteral(ranges, i)) {
            return line.substr(0, i);
        }
    }
    return line;
}

// Мутациони оператори

struct TokenOperator {
    std::string from;
    std::string to;
    std::string op;
};

// оператори од два знака можемо одмах замјенити 
const std::vector<TokenOperator> kTwoCharOperators = {
    {"<=", "<",  "ROR"}, {">=", ">",  "ROR"},
    {"==", "!=", "ROR"}, {"!=", "==", "ROR"},
    {"&&", "||", "LCR"}, {"||", "&&", "LCR"},
    {"+=", "-=", "AOR"}, {"-=", "+=", "AOR"},
    {"++", "--", "AOR"},
};

// оператори од једног знака, мијењају се само ако су окружени размацима јер у супротном може да се деси да не служе за поређење
//нпр показивач ->
const std::vector<TokenOperator> kSpacedOperators = {
    {"<", "<=", "ROR"}, {">", ">=", "ROR"},
    {"+", "-",  "AOR"}, {"-", "+",  "AOR"},
    {"*", "/",  "AOR"}, {"/", "*",  "AOR"},
};

const std::regex kIfPattern(R"(^(\s*(?:else\s+)?if\s*)\((.*)\)(\s*\{?\s*)$)");
const std::regex kStatementPattern(
    R"(^[A-Za-z_][\w.\->:\[\]]*\s*(\(.*\)|\+\+|--|[-+*/]?=\s*.+);$)");
const std::vector<std::string> kStatementSkipPrefixes = {
    "return", "break", "throw", "continue"};

bool twoCharAllowed(const std::string& code, std::size_t pos, const std::string& token) {
    const char prev = pos > 0 ? code[pos - 1] : ' ';
    const char next = pos + 2 < code.size() ? code[pos + 2] : ' ';
    if (token == "<=") return prev != '<' && next != '>';   // не <<= ни <=>
    if (token == ">=") return prev != '>';                  // не >>=
    if (token == "==") return prev != '=' && prev != '!' && prev != '<' && prev != '>';
    return true;
}

std::vector<LineMutation> mutateLine(const std::string& line) {
    std::vector<LineMutation> result;
    const std::string trimmed = trim(line);
    if (trimmed.empty() || startsWith(trimmed, "#") || startsWith(trimmed, "//") ||
        startsWith(trimmed, "/*") || startsWith(trimmed, "*")) {
        return result;
    }

    const std::string code = codePart(line);
    const std::string rest = line.substr(code.size());
    const auto literals = literalRanges(code);

    auto add = [&](const std::string& op, const std::string& description,
                   std::size_t pos, std::size_t length, const std::string& replacement) {
        if (insideLiteral(literals, pos)) return;
        result.push_back({op, description,
                          code.substr(0, pos) + replacement + code.substr(pos + length) + rest});
    };

    //оператори од два знака
    for (const auto& t : kTwoCharOperators) {
        for (std::size_t pos = code.find(t.from); pos != std::string::npos;
             pos = code.find(t.from, pos + 1)) {
            if (twoCharAllowed(code, pos, t.from)) {
                add(t.op, t.from + " у " + t.to, pos, t.from.size(), t.to);
            }
        }
    }

    //  оператори од једног знака 
    for (const auto& t : kSpacedOperators) {
        for (std::size_t pos = code.find(t.from); pos != std::string::npos;
             pos = code.find(t.from, pos + 1)) {
            if (pos > 0 && pos + 1 < code.size() && isSpace(code[pos - 1]) &&
                isSpace(code[pos + 1])) {
                add(t.op, t.from + " у " + t.to, pos, 1, t.to);
            }
        }
    }

    // логичка негације 
    for (std::size_t pos = code.find('!'); pos != std::string::npos;
         pos = code.find('!', pos + 1)) {
        const char prev = pos > 0 ? code[pos - 1] : ' ';
        const char next = pos + 1 < code.size() ? code[pos + 1] : ' ';
        if (next != '=' && prev != '=' && prev != '!' && prev != '<' && prev != '>') {
            add("UOD", "уклањање !", pos, 1, "");
        }
    }

    // true и false
    for (const auto& [from, to] : std::vector<std::pair<std::string, std::string>>{
             {"true", "false"}, {"false", "true"}}) {
        for (std::size_t pos = code.find(from); pos != std::string::npos;
             pos = code.find(from, pos + 1)) {
            const bool startOk = pos == 0 || !isWordChar(code[pos - 1]);
            const bool endOk = pos + from.size() >= code.size() ||
                               !isWordChar(code[pos + from.size()]);
            if (startOk && endOk) add("BCR", from + " у " + to, pos, from.size(), to);
        }
    }

    // негација цијелог услова у if 
    std::smatch ifMatch;
    if (std::regex_match(code, ifMatch, kIfPattern)) {
        result.push_back({"NCC", "негација услова",
                          ifMatch[1].str() + "(!(" + ifMatch[2].str() + "))" +
                              ifMatch[3].str() + rest});
    }

    // број N постаје N + 1 
    for (std::size_t i = 0; i < code.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(code[i]))) continue;
        if (i > 0 && (isWordChar(code[i - 1]) || code[i - 1] == '.')) {
            while (i < code.size() && isWordChar(code[i])) ++i;  // дио идентификатора
            continue;
        }
        std::size_t j = i;
        while (j < code.size() && std::isdigit(static_cast<unsigned char>(code[j]))) ++j;
        const std::string whole = code.substr(i, j - i);
        std::string fraction;
        if (j + 1 < code.size() && code[j] == '.' &&
            std::isdigit(static_cast<unsigned char>(code[j + 1]))) {
            std::size_t k = j + 1;
            while (k < code.size() && std::isdigit(static_cast<unsigned char>(code[k]))) ++k;
            fraction = code.substr(j, k - j);
            j = k;
        }
        const bool suffix = j < code.size() && (isWordChar(code[j]) || code[j] == '.');
        if (!suffix && whole.size() < 10) {  // прескочи 1.0f, 10u, 0x1F, огромне бројеве
            const std::string next = std::to_string(std::stoll(whole) + 1) + fraction;
            add("CRP", whole + fraction + " у " + next, i, j - i, next);
        }
        i = j;
    }

    // брисање наредбе
    const std::string statement = trim(code);
    bool skip = false;
    for (const auto& prefix : kStatementSkipPrefixes) {
        if (startsWith(statement, prefix)) skip = true;
    }
    if (!skip && std::regex_match(statement, kStatementPattern)) {
        result.push_back({"SDL", "брисање наредбе",
                          indentOf(line) + "/* MUTANT: " + statement + " */"});
    }

    return result;
}

std::vector<Mutant> generateMutants(const std::vector<fs::path>& files, const fs::path& root) {
    std::vector<Mutant> mutants;
    for (const auto& path : files) {
        const auto lines = splitKeepEndings(readBytes(path));
        bool inBlockComment = false; //провјера да ли је у блок коментару та линија
        for (std::size_t index = 0; index < lines.size(); ++index) {
            const std::string line = withoutEnding(lines[index]);
            const std::string trimmed = trim(line);
            if (inBlockComment) {
                if (trimmed.find("*/") != std::string::npos) inBlockComment = false;
                continue;
            }
            if (startsWith(trimmed, "/*") && trimmed.find("*/") == std::string::npos) {
                inBlockComment = true;
                continue;
            }
            for (const auto& m : mutateLine(line)) {
                Mutant mutant;
                mutant.id = static_cast<int>(mutants.size()) + 1;
                mutant.file = fs::relative(path, root).generic_string();
                mutant.line = static_cast<int>(index) + 1;
                mutant.op = m.op;
                mutant.description = m.description;
                mutant.original = trimmed;
                mutant.mutatedLine = m.newLine;
                mutants.push_back(mutant);
            }
        }
    }
    return mutants;
}

// Компајлирање и покретање тестова

std::string quote(const fs::path& path) { return "\"" + path.generic_string() + "\""; }

// Покреће команду преко std::system и њен излаз преусмјерава у log фајл.
// GCOVR_EXCL_START
// Танак омотач око std::system: једини дио алата који стварно покреће команде.
// Не тестира се јединично, тестови умјесто њега користе MockCommandRunner.
int SystemCommandRunner::run(const std::string& command, const fs::path& log) {
    const std::string full = command + " > " + quote(log) + " 2>&1";
    return std::system(full.c_str());
}
// GCOVR_EXCL_STOP

// Да ли је команда прекинута са Ctrl+C.
// std::system на Linuxу игнорише Ctrl+C у овом процесу док чека команду,
// па се прекид препознаје по томе како је команда завршила.
bool interruptedByCtrlC(int status) {
#ifdef _WIN32
    return static_cast<unsigned int>(status) == 0xC000013Au;  // STATUS_CONTROL_C_EXIT
#else
    const bool killedBySigint = WIFSIGNALED(status) && WTERMSIG(status) == SIGINT;
    const bool exitedWith130 = WIFEXITED(status) && WEXITSTATUS(status) == 128 + SIGINT;
    return killedBySigint || exitedWith130;
#endif
}

// Покреће команду, излаз пише у log фајл. Враћа true ако је успјела.
bool runCommand(CommandRunner& runner, const std::string& command, const fs::path& log) {
    const int status = runner.run(command, log);
    if (interruptedByCtrlC(status)) g_interrupted = true;
    return status == 0;
}

// Гради само тестове (и библиотеку од које зависе), а не цијели пројекат.
// Да се гради све, cmake би покушао поново да линкује и mutation_tester.exe,
// а на Windowsу се извршни фајл који је тренутно покренут не може пребрисати.
bool build(CommandRunner& runner, const fs::path& buildDir) {
    return runCommand(runner,
                      "cmake --build " + quote(buildDir) + " --target order_system_tests",
                      buildDir / "mutation_build.log");
}

// може вратити "passed", "failed" или "timeout".
std::string runTests(CommandRunner& runner, const fs::path& buildDir) {
    const fs::path log = buildDir / "mutation_test.log";
    const std::string command = "ctest --test-dir " + quote(buildDir) +
                                " --stop-on-failure --timeout 30"
                                " -E \"^MutationTool\\.\"";  // тестови самог алата се не покрећу
    if (runCommand(runner, command, log)) return "passed";
    return readBytes(log).find("Timeout") != std::string::npos ? "timeout" : "failed";
}

// име првог теста који је пао, из ctest излаза:
//   The following tests FAILED:
//         45 - CartTest.AddingSameProductIncreasesQuantity (Failed)
std::string firstFailedTest(const fs::path& buildDir) {
    const std::string log = readBytes(buildDir / "mutation_test.log");
    std::size_t pos = log.find("tests FAILED:");
    if (pos == std::string::npos) return "";
    pos = log.find(" - ", pos);
    if (pos == std::string::npos) return "";
    const std::size_t end = log.find(" (", pos);
    if (end == std::string::npos) return "";
    return log.substr(pos + 3, end - pos - 3);
}

// Конфигурише build фолдер ако још није. Баца std::runtime_error ако не успије.
void ensureConfigured(CommandRunner& runner, const fs::path& buildDir, const fs::path& root) {
    if (fs::exists(buildDir / "CMakeCache.txt")) return;
    std::cout << "Build фолдер " << buildDir.generic_string()
              << " није конфигурисан, конфигуришем...\n";
    fs::create_directories(buildDir);
    const bool usePreset =
        fs::exists(root / "CMakePresets.json") &&
        fs::weakly_canonical(buildDir) == fs::weakly_canonical(root / "build" / "debug");
    std::string command;
    if (usePreset) {
        command = "cmake --preset debug";
    } else {
        command = "cmake -S " + quote(root) + " -B " + quote(buildDir) +
                  " -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON";
    }
    if (!runCommand(runner, command, buildDir / "mutation_configure.log")) {
        throw std::runtime_error("Конфигурисање није успјело, видјети mutation_configure.log");
    }
}

// Враћа изворни фајл у оригинално стање кад изађе из опсега 
// без обзира да ли је тест прошао, пао или је бачен изузетак.
class FileRestorer {
public:
    FileRestorer(fs::path path, std::string original)
        : path_(std::move(path)), original_(std::move(original)) {}
    ~FileRestorer() { writeBytes(path_, original_); }
    FileRestorer(const FileRestorer&) = delete;
    FileRestorer& operator=(const FileRestorer&) = delete;

private:
    fs::path path_;
    std::string original_;
};

void testMutant(Mutant& mutant, const fs::path& root, const fs::path& buildDir,
                CommandRunner& runner) {
    const fs::path path = root / mutant.file;
    const std::string originalBytes = readBytes(path);
    FileRestorer restorer(path, originalBytes);

    auto lines = splitKeepEndings(originalBytes);
    std::string& line = lines[mutant.line - 1];
    line = mutant.mutatedLine + lineEnding(line);

    std::string mutated;
    for (const auto& l : lines) mutated += l;
    writeBytes(path, mutated);

    if (!build(runner, buildDir)) {
        mutant.status = "не компајлира се";
    } else {
        const std::string outcome = runTests(runner, buildDir);
        mutant.status = outcome == "passed"    ? "преживио"
                        : outcome == "timeout" ? "убијен (timeout)"
                                               : "убијен";
        if (outcome != "passed") mutant.killedBy = firstFailedTest(buildDir);
    }
    if (g_interrupted) mutant.status = "непокренут";  // резултат прекинутог покретања не важи
}

// Извјештај

bool isKilled(const Mutant& m) { return startsWith(m.status, "убијен"); }

Score score(const std::vector<const Mutant*>& mutants) {
    Score s;
    for (const auto* m : mutants) {
        if (isKilled(*m)) ++s.killed;
        if (m->status == "преживио") ++s.survived;
    }
    const int total = s.killed + s.survived;
    s.percent = total > 0 ? 100.0 * s.killed / total : 0.0;
    return s;
}

std::string percent(double value) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << value << "%";
    return out.str();
}

// Усправна црта у ћелији табеле мора бити escape-ована 
std::string cell(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (c == '|') out += '\\';
        out += c;
    }
    return out;
}

std::string createReport(const std::vector<Mutant>& mutants) {
    std::vector<const Mutant*> all;
    for (const auto& m : mutants) all.push_back(&m);
    const Score total = score(all);
    int notCompiled = 0;
    for (const auto& m : mutants) notCompiled += m.status == "не компајлира се";

    std::ostringstream r;
    r << "# Извјештај мутационог тестирања\n\n"
      << "- Укупно мутаната: **" << mutants.size() << "**\n"
      << "- Убијени: **" << total.killed << "**\n"
      << "- Преживјели: **" << total.survived << "**\n"
      << "- Не компајлирају се (не рачунају се): **" << notCompiled << "**\n"
      << "- Мутациони скор: **" << percent(total.percent) << "**\n\n";

    auto groupTable = [&](const std::string& title, const std::string& column,
                          auto keyOf) {
        std::map<std::string, std::vector<const Mutant*>> groups;
        for (const auto& m : mutants) groups[keyOf(m)].push_back(&m);
        r << "## " << title << "\n\n"
          << "| " << column << " | Мутанти | Убијени | Преживјели | Скор |\n"
          << "|---|---:|---:|---:|---:|\n";
        for (const auto& [key, group] : groups) {
            const Score s = score(group);
            r << "| " << key << " | " << group.size() << " | " << s.killed << " | "
              << s.survived << " | " << percent(s.percent) << " |\n";
        }
        r << "\n";
    };
    groupTable("По фајловима", "Фајл", [](const Mutant& m) { return m.file; });
    groupTable("По операторима", "Оператор", [](const Mutant& m) { return m.op; });

    r << "## Преживјели мутанти\n\n";
    bool anySurvived = false;
    for (const auto& m : mutants) anySurvived |= m.status == "преживио";
    if (!anySurvived) {
        r << "Нема преживјелих мутаната.\n\n";
    } else {
        r << "| # | Фајл:линија | Оператор | Оригинал | Мутант |\n"
          << "|---:|---|---|---|---|\n";
        for (const auto& m : mutants) {
            if (m.status != "преживио") continue;
            r << "| " << m.id << " | " << m.file << ":" << m.line << " | " << m.op << " ("
              << cell(m.description) << ") | `" << cell(m.original) << "` | `"
              << cell(trim(m.mutatedLine)) << "` |\n";
        }
        r << "\n";
    }

    r << "## Сви мутанти\n\n"
      << "| # | Фајл:линија | Оператор | Мутант | Статус | Убио га тест |\n"
      << "|---:|---|---|---|---|---|\n";
    for (const auto& m : mutants) {
        r << "| " << m.id << " | " << m.file << ":" << m.line << " | " << m.op << " ("
          << cell(m.description) << ") | `" << cell(trim(m.mutatedLine)) << "` | " << m.status
          << " | " << m.killedBy << " |\n";
    }
    r << "\nПреживјели мутант не значи обавезно лош тест: може бити еквивалентан "
         "мутант (измјена која не мијења понашање програма). Сваки преживјели "
         "мутант треба анализирати.\n";
    return r.str();
}

// ---------------------------------------------------------------------------
// Планиране мутације (--plan)
// ---------------------------------------------------------------------------
//
// Формат фајла:
// нпр
//   [M01] Опис мутације и зашто је занимљива
//   file:     src/Cart.cpp
//   original: quantity <= 0
//   mutant:   quantity < 0
//   expected: killed          (или equivalent ако очекујемо да преживи)
//


std::vector<Mutant> loadPlan(const fs::path& planPath, const fs::path& root) {
    if (!fs::exists(planPath)) {
        throw std::runtime_error("Фајл са планом не постоји: " + planPath.generic_string());
    }

    std::vector<Mutant> mutants;
    std::map<std::string, std::string> fields;
    std::string id, title;
    std::ostringstream errors;  // све грешке у плану, да се пријаве одједном

    auto finish = [&]() {
        if (id.empty()) return;
        const std::string file = fields["file"];
        const std::string original = fields["original"];
        const std::string mutant = fields["mutant"];
        std::string expected = fields.count("expected") ? fields["expected"] : "killed";
        if (file.empty() || original.empty()) {
            errors << "[" << id << "] недостаје file или original\n";
            return;
        }
        if (expected != "killed" && expected != "equivalent") {
            errors << "[" << id << "] expected мора бити killed или equivalent\n";
            return;
        }
        const fs::path path = root / file;
        if (!fs::exists(path)) {
            errors << "[" << id << "] фајл не постоји: " << file << "\n";
            return;
        }

        // тражи линију кода (не коментар) која садржи original.
        const auto lines = splitKeepEndings(readBytes(path));
        std::vector<std::size_t> matches;
        bool inBlockComment = false;
        for (std::size_t i = 0; i < lines.size(); ++i) {
            const std::string line = withoutEnding(lines[i]);
            const std::string trimmed = trim(line);
            if (inBlockComment) {
                if (trimmed.find("*/") != std::string::npos) inBlockComment = false;
                continue;
            }
            if (startsWith(trimmed, "/*")) {
                inBlockComment = trimmed.find("*/") == std::string::npos;
                continue;
            }
            if (codePart(line).find(original) != std::string::npos) matches.push_back(i);
        }
        if (matches.size() != 1) {
            errors << "[" << id << "] \"" << original << "\" се у " << file << " појављује "
                   << matches.size() << " пута (мора тачно једном)\n";
            return;
        }

        const std::string line = withoutEnding(lines[matches[0]]);
        const std::size_t pos = line.find(original);
        Mutant m;
        m.id = static_cast<int>(mutants.size()) + 1;
        m.file = file;
        m.line = static_cast<int>(matches[0]) + 1;
        m.op = id;
        m.description = title;
        m.original = trim(line);
        m.mutatedLine = line.substr(0, pos) + mutant + line.substr(pos + original.size());
        m.expected = expected;
        mutants.push_back(m);
    };

    std::istringstream in(readBytes(planPath));
    std::string raw;
    while (std::getline(in, raw)) {
        const std::string line = trim(raw);
        if (line.empty() || startsWith(line, "#")) continue;
        if (line.front() == '[') {
            finish();
            const std::size_t close = line.find(']');
            id = close == std::string::npos ? line : line.substr(1, close - 1);
            title = close == std::string::npos ? "" : trim(line.substr(close + 1));
            fields.clear();
            continue;
        }
        const std::size_t colon = line.find(':');
        if (colon == std::string::npos || id.empty()) {
            errors << "Неисправна линија у плану: " << line << "\n";
            continue;
        }
        fields[trim(line.substr(0, colon))] = trim(line.substr(colon + 1));
    }
    finish();

    if (!errors.str().empty()) throw std::runtime_error(errors.str());
    return mutants;
}

std::string createPlanReport(const std::vector<Mutant>& mutants) {
    std::vector<const Mutant*> all;
    for (const auto& m : mutants) all.push_back(&m);
    const Score total = score(all);

    int asExpected = 0, equivalentCount = 0;
    for (const auto& m : mutants) {
        const bool survived = m.status == "преживио";
        if ((m.expected == "killed" && isKilled(m)) ||
            (m.expected == "equivalent" && survived)) {
            ++asExpected;
        }
        if (m.expected == "equivalent") ++equivalentCount;
    }
    const int nonEquivalent = static_cast<int>(mutants.size()) - equivalentCount;
    const double nonEquivalentScore =
        nonEquivalent > 0 ? 100.0 * total.killed / nonEquivalent : 0.0;

    std::ostringstream r;
    r << "# Извјештај планираних мутација\n\n";
    r << "- Планираних мутација: **" << mutants.size() << "**\n";
    r << "- Убијени: **" << total.killed << "**\n";
    r << "- Преживјели: **" << total.survived << "**\n";
    r << "- Мутациони скор: **" << percent(total.percent) << "**\n";
    r << "- Скор без еквивалентних мутаната: **" << percent(nonEquivalentScore) << "**\n";
    r << "- Резултат као што је очекивано: **" << asExpected << " / " << mutants.size()
      << "**\n\n"
      << "| ID | Опис | Фајл:линија | Оригинал | Мутант | Очекивано | Резултат | Убио га тест |\n"
      << "|---|---|---|---|---|---|---|---|\n";
    for (const auto& m : mutants) {
        const bool match = (m.expected == "killed" && isKilled(m)) ||
                           (m.expected == "equivalent" && m.status == "преживио");
        r << "| " << m.op << " | " << cell(m.description) << " | " << m.file << ":" << m.line
          << " | `" << cell(m.original) << "` | "
          << (trim(m.mutatedLine).empty() ? "*(обрисано)*"
                                           : "`" + cell(trim(m.mutatedLine)) + "`")
          << " | " << (m.expected == "killed" ? "убијен" : "еквивалентан") << " | " << m.status
          << (match ? " ✓" : " ✗") << " | " << m.killedBy << " |\n";
    }
    r << "\n✓ = резултат се поклапа са очекивањем, ✗ = не поклапа се "
         "(тестови не уочавају грешку, или мутант није еквивалентан како се мислило).\n";
    return r.str();
}

}  // namespace mutation