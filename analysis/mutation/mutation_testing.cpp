//
// за сваки мутант поново компајлира тестове и покреће их:
//   - ако бар један тест падне, мутант је "убијен";
//   - ако сви тестови прођу, мутант је "преживио".
// Мутациони скор = убијени / (убијени + преживјели).
//


#include "mutation_core.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// CMake прослијеђује ове вриједности
#ifndef PROJECT_ROOT
#define PROJECT_ROOT "."
#endif
#ifndef DEFAULT_BUILD_DIR
#define DEFAULT_BUILD_DIR "build/debug"
#endif

using namespace mutation;

namespace {

void printUsage() {
    std::cout << "Коришћење: mutation_tester [--file ТЕКСТ] [--plan ФАЈЛ] [--list] [--build-dir ФОЛДЕР]\n"
                 "  --file ТЕКСТ        само фајлови из src/ чије име садржи ТЕКСТ\n"
                 "  --plan ФАЈЛ         само ручно планиране мутације из ФАЈЛ-а\n"
                 "  --list              само испиши мутанте, без покретања\n"
                 "  --build-dir ФОЛДЕР  build фолдер (подразумијевано "
              << DEFAULT_BUILD_DIR << ")\n";
}

// Цијели ток алата
// Изузеци из mutation_core хвата main()
int runTool(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8); 
#endif

    SystemCommandRunner runner;
    const fs::path root = fs::absolute(PROJECT_ROOT);
    fs::path buildDir = DEFAULT_BUILD_DIR;
    std::string fileFilter;
    fs::path planPath;
    bool listOnly = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            listOnly = true;
        } else if (arg == "--file" && i + 1 < argc) {
            fileFilter = argv[++i];
        } else if (arg == "--plan" && i + 1 < argc) {
            planPath = argv[++i];
        } else if (arg == "--build-dir" && i + 1 < argc) {
            buildDir = argv[++i];
        } else {
            printUsage();
            return arg == "--help" || arg == "-h" ? 0 : 1;
        }
    }
    if (planPath.empty() == false && planPath.is_relative()) planPath = fs::absolute(planPath);
    if (buildDir.is_relative()) buildDir = root / buildDir;
    fs::current_path(root);

    // фајлови за мутирање су сви из src/ осим mainа
    std::vector<fs::path> files;
    for (const auto& entry : fs::directory_iterator(root / "src")) {
        const std::string name = entry.path().filename().string();
        if (entry.path().extension() != ".cpp" || name == "main.cpp") continue;
        std::string lowerName = name, lowerFilter = fileFilter;
        for (auto& c : lowerName) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        for (auto& c : lowerFilter) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (lowerName.find(lowerFilter) != std::string::npos) files.push_back(entry.path());
    }
    std::sort(files.begin(), files.end());
    if (files.empty()) {
        std::cerr << "Нема фајлова за мутирање.\n";
        return 1;
    }

    const bool planMode = !planPath.empty();
    auto mutants = planMode ? loadPlan(planPath, root) : generateMutants(files, root);
    if (planMode) {
        std::cout << "Учитано " << mutants.size() << " планираних мутација.\n";
    } else {
        std::cout << "Генерисано " << mutants.size() << " мутаната у " << files.size()
                  << " фајлова.\n";
    }

    if (listOnly) {
        for (const auto& m : mutants) {
            std::cout << std::setw(4) << m.id << " " << m.file << ":" << std::left
                      << std::setw(4) << m.line << " " << std::setw(4) << m.op << std::right
                      << " " << trim(m.mutatedLine) << "\n";
        }
        return 0;
    }

    ensureConfigured(runner, buildDir, root);

    std::cout << "Провјера оригиналног кода...\n";
    if (!build(runner, buildDir)) {
        std::cerr << "Оригинални код се не компајлира, видјети "
                  << (buildDir / "mutation_build.log").generic_string() << "\n";
        return 1;
    }
    if (runTests(runner, buildDir) != "passed") {
        std::cerr << "Тестови не пролазе на оригиналном коду. Прво их поправи.\n";
        return 1;
    }

    std::signal(SIGINT, onInterrupt);
    const auto start = std::chrono::steady_clock::now();

    for (auto& m : mutants) {
        if (g_interrupted) break;
        testMutant(m, root, buildDir, runner);
        if (g_interrupted) break;
        std::cout << "[" << std::setw(3) << m.id << "/" << mutants.size() << "] "
                  << std::left << std::setw(18) << m.status << std::right << " " << m.file
                  << ":" << m.line << "  " << trim(m.mutatedLine) << std::endl;
    }
    if (g_interrupted) {
        std::cout << "\nПрекинуто. Изворни фајлови су враћени у оригинално стање.\n";
    }

    build(runner, buildDir);  // поново компајлирај оригинални код

    std::vector<Mutant> tested;
    for (const auto& m : mutants) {
        if (m.status != "непокренут") tested.push_back(m);
    }
    const fs::path reportPath =
        root / "analysis" / "mutation" / (planMode ? "planned_mutation_report.md" : "mutation_report.md");
    writeBytes(reportPath, planMode ? createPlanReport(tested) : createReport(tested));

    std::vector<const Mutant*> all;
    for (const auto& m : tested) all.push_back(&m);
    const Score total = score(all);
    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
                             std::chrono::steady_clock::now() - start).count();
    std::cout << "\nУбијено " << total.killed << ", преживјело " << total.survived
              << ", скор " << percent(total.percent) << " (" << seconds << " s)\n"
              << "Извјештај: " << reportPath.generic_string() << "\n";
    return 0;
}

}  

int main(int argc, char* argv[]) {
    try {
        return runTool(argc, argv);
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}