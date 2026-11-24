"""
    branches        -if, for, while, case, catch, ?:, &&, ||
    parameters      - broj ulaznih parametara
    dependencies    - broj zavisnosti (clanovi tipa referenca/pokazivac na drugu klasu, npr. IInventoryService&)
    side_effects    - upis u clanove klase i pozivi ne-const metoda zavisnosti
    error_handling  - obrada gresaka: povratne vrijednosti koje oznacavaju gresku (return false, return OrderResult::InvalidNesto...)
    exceptions      - throw i catch
    mocking         - potreba za mock objektima: broj zavisnosti ciji je tip interfejs (apstraktna klasa)

Za faktore branches, side_effects, error_handling i exceptions uracunavaju se i metode iste klase koje funkcija poziva
(npr. applyDiscount poziva calculateDiscountPercentage) jer i one moraju proci kroz test.

Svaki izbrojani faktor se preslikava u nivo 0-3 (ima tabela THRESHOLDS), zbir nivoa je broj bodova,
 a bodovi se preslikavaju u ocjenu od 1 do 5.

"""

from __future__ import annotations

import argparse
import csv
import re
from dataclasses import dataclass, field
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_DIRECTORY = Path(__file__).resolve().parent

FACTORS = [
    "branches",
    "parameters",
    "dependencies",
    "side_effects",
    "error_handling",
    "exceptions",
    "mocking",
]

FACTOR_NAMES = {
    "branches": "Гранања",
    "parameters": "Параметри",
    "dependencies": "Зависности",
    "side_effects": "Споредни ефекти",
    "error_handling": "Обрада грешака",
    "exceptions": "Изузеци",
    "mocking": "Mock објекти",
}

# Granice za nivoe:
# vrijednost <= prve granice  -> 0. nivo
# vrijednost <= druge granice -> 1. nivo
# vrijednost <= trece granice -> 2. nivo
# vise od toga -> 3. nivo.
THRESHOLDS = {
    "branches": (0, 2, 4),
    "parameters": (0, 1, 2),
    "dependencies": (0, 1, 2),
    "side_effects": (0, 1, 3),
    "error_handling": (0, 1, 3),
    "exceptions": (0, 1, 2),
    "mocking": (0, 1, 3),
}

MUTATING_METHODS = {
    "push_back", "emplace_back", "pop_back", "push", "pop", "emplace",
    "insert", "erase", "clear", "resize", "assign", "swap",
}

ERROR_WORDS = re.compile(r"Invalid|Fail|Error|Unavailable")


# STRUCTURES

@dataclass
class Member:
    name: str
    type_name: str

    @property
    def base_type(self) -> str:
        cleaned = re.sub(r"\b(const|volatile)\b|[&*]", " ", self.type_name)
        return cleaned.split()[-1] if cleaned.split() else ""

    @property
    def is_reference_or_pointer(self) -> bool:
        return "&" in self.type_name or "*" in self.type_name


@dataclass
class ClassInfo:
    name: str
    members: dict[str, Member] = field(default_factory=dict)
    methods: dict[str, bool] = field(default_factory=dict)  # ime -> da li je const
    is_interface: bool = False


@dataclass
class Function:
    class_name: str
    name: str
    parameters: int
    body: str
    is_constructor: bool = False
    initializers: str = "" 


@dataclass
class Result:
    class_name: str
    function_name: str
    raw: dict[str, int]
    levels: dict[str, int]

    @property
    def score(self) -> int:
        return sum(self.levels.values())

    @property
    def grade(self) -> int:
        return difficulty_grade(self.score)


# Pomocne funk za ciscenje koda

def strip_comments_and_strings(code: str) -> str:
    """Uklanja komentare i sadrzaj string/char literala (ostaju prazni navodnici)."""
    result = []
    i = 0
    length = len(code)
    while i < length:
        c = code[i]
        nxt = code[i + 1] if i + 1 < length else ""
        if c == "/" and nxt == "/":
            end = code.find("\n", i)
            i = length if end == -1 else end
        elif c == "/" and nxt == "*":
            end = code.find("*/", i + 2)
            i = length if end == -1 else end + 2
            result.append(" ")
        elif c in "\"'":
            j = i + 1
            while j < length and code[j] != c:
                j += 2 if code[j] == "\\" else 1
            result.append(c + c)
            i = j + 1
        else:
            result.append(c)
            i += 1
    return "".join(result)


def matching(code: str, start: int, open_char: str, close_char: str) -> int:
    """Indeks zagrade koja zatvara zagradu na poziciji start."""
    depth = 0
    for i in range(start, len(code)):
        if code[i] == open_char:
            depth += 1
        elif code[i] == close_char:
            depth -= 1
            if depth == 0:
                return i
    raise ValueError(f"nezatvorena zagrada '{open_char}' na poziciji {start}")


def split_top_level(text: str, separator: str) -> list[str]:
    """Dijeli tekst po separatoru, ali ne unutar (), <>, {} i []."""
    parts, depth, current = [], 0, []
    for c in text:
        if c in "(<{[":
            depth += 1
        elif c in ")>}]":
            depth -= 1
        if c == separator and depth == 0:
            parts.append("".join(current))
            current = []
        else:
            current.append(c)
    parts.append("".join(current))
    return parts


def count_parameters(parameter_list: str) -> int:
    text = parameter_list.strip()
    if not text or text == "void":
        return 0
    return len(split_top_level(text, ","))


# HEADERS: klase, clanovi, metode

def parse_header(code: str) -> list[ClassInfo]:
    code = strip_comments_and_strings(code)
    classes = []
    for match in re.finditer(r"(?<!enum\s)\b(class|struct)\s+(\w+)[^;{]*\{", code):
        if re.search(r"\benum\s+$", code[: match.start()]):
            continue
        open_brace = match.end() - 1
        close_brace = matching(code, open_brace, "{", "}")
        body = code[open_brace + 1 : close_brace]
        classes.append(parse_class_body(match.group(2), body))
    return classes


def parse_class_body(name: str, body: str) -> ClassInfo:
    info = ClassInfo(name=name)
    body = re.sub(r"\b(public|private|protected)\s*:", " ", body)
    pure_virtual = 0
    for declaration in split_top_level(body, ";"):
        declaration = " ".join(declaration.split())
        if not declaration or "{" in declaration:
            continue  # ugnijezdjena struktura ili inline tijelo
        if "(" in declaration:
            method = re.search(r"(~?\w+)\s*\(", declaration)
            if not method or method.group(1).startswith("~"):
                continue
            after = declaration[declaration.rfind(")") + 1 :]
            info.methods[method.group(1)] = bool(re.search(r"\bconst\b", after))
            if re.search(r"=\s*0\s*$", declaration):
                pure_virtual += 1
        else:
            member = re.match(r"(.+?)\s*\b(\w+)\s*(=.*)?$", declaration)
            if member and not member.group(1).strip().startswith(("using", "friend", "typedef")):
                info.members[member.group(2)] = Member(member.group(2), member.group(1).strip())
    info.is_interface = pure_virtual > 0 and not info.members
    return info


# SRC FILES: definicije funkcija

def parse_source(code: str) -> list[Function]:
    code = strip_comments_and_strings(code)
    functions = []
    position = 0
    pattern = re.compile(r"\b(\w+)::(~?\w+)\s*\(")
    while True:
        match = pattern.search(code, position)
        if not match:
            break
        if code[: match.start()].count("{") != code[: match.start()].count("}"):
            position = match.end()  # nije na najvisem nivou (poziv unutar tijela)
            continue
        open_paren = match.end() - 1
        close_paren = matching(code, open_paren, "(", ")")
        body_start = find_body_start(code, close_paren + 1)
        if body_start is None:
            position = close_paren + 1  # deklaracija, ne definicija
            continue
        body_end = matching(code, body_start, "{", "}")
        class_name, name = match.group(1), match.group(2)
        if not name.startswith("~"):
            functions.append(Function(
                class_name=class_name,
                name=name,
                parameters=count_parameters(code[open_paren + 1 : close_paren]),
                body=code[body_start + 1 : body_end],
                is_constructor=class_name == name,
                initializers=code[close_paren + 1 : body_start] if class_name == name else "",
            ))
        position = body_end + 1
    return functions


def find_body_start(code: str, index: int) -> int | None:
    """Preskace kvalifikatore (const, override...) i inicijalizacionu listu
    konstruktora i vraca poziciju '{' kojom pocinje tijelo funkcije."""
    qualifiers = re.compile(r"\s*(?:(?:const|override|final|noexcept)\b|&&|&)")
    while True:
        match = qualifiers.match(code, index)
        if not match or match.end() == index:
            break
        index = match.end()
    while index < len(code) and code[index].isspace():
        index += 1
    if index < len(code) and code[index] == ":":
        index += 1
        while True:
            initializer = re.compile(r"\s*[\w:]+\s*([({])").match(code, index)
            if not initializer:
                return None
            bracket = initializer.end() - 1
            close = ")" if code[bracket] == "(" else "}"
            index = matching(code, bracket, code[bracket], close) + 1
            comma = re.compile(r"\s*,").match(code, index)
            if not comma:
                break
            index = comma.end()
        while index < len(code) and code[index].isspace():
            index += 1
    return index if index < len(code) and code[index] == "{" else None


# FACTORS

def count_decisions(body: str) -> int:
    keywords = len(re.findall(r"\b(if|for|while|case|catch)\b", body))
    logical = len(re.findall(r"&&|\|\|", body))
    ternary = len(re.findall(r"\?(?!\?)", body))
    return keywords + logical + ternary


def count_error_handling(body: str) -> int:
    """Povratne vrijednosti koje signaliziraju gresku."""
    error_returns = len(re.findall(r"\breturn\s+false\s*;", body))
    for value in re.findall(r"\breturn\s+\w+::(\w+)\s*;", body):
        if ERROR_WORDS.search(value):
            error_returns += 1
    return error_returns


def count_exceptions(body: str) -> int:
    """Bacanje i hvatanje izuzetaka."""
    return len(re.findall(r"\b(throw|catch)\b", body))


def written_members(body: str, members: dict[str, Member]) -> set[str]:
    written = set()
    for name in members:
        n = re.escape(name)
        patterns = [
            rf"\b{n}\s*(\[[^\]]*\])?\s*(?:[+\-*/%|&^]?=(?!=)|\+\+|--)",
            rf"(?:\+\+|--)\s*{n}\b",
            rf"\b{n}\s*\.\s*(?:{'|'.join(MUTATING_METHODS)})\s*\(",
        ]
        if any(re.search(p, body) for p in patterns):
            written.add(name)
    return written


def own_method_calls(body: str, class_info: ClassInfo) -> set[str]:
    calls = set()
    for name in re.findall(r"(?<![\w.>:])(\w+)\s*\(", body):
        if name in class_info.methods:
            calls.add(name)
    return calls


def dependency_members(class_info: ClassInfo, classes: dict[str, ClassInfo]) -> dict[str, Member]:
    return {
        name: member
        for name, member in class_info.members.items()
        if member.is_reference_or_pointer and member.base_type in classes
    }


class Analyzer:
    def __init__(self, classes: dict[str, ClassInfo], functions: list[Function]):
        self.classes = classes
        self.functions = {(f.class_name, f.name): f for f in functions}
        self.order = functions

    def analyze_all(self) -> list[Result]:
        return [self.analyze(f) for f in self.order]

    def analyze(self, function: Function) -> Result:
        class_info = self.classes.get(function.class_name, ClassInfo(function.class_name))
        dependencies = dependency_members(class_info, self.classes)

        bodies = self.reachable_bodies(function, class_info)
        body = "\n".join(bodies)

        dependency_text = body + "\n" + function.initializers
        used = {name for name in dependencies if re.search(rf"\b{re.escape(name)}\b", dependency_text)}       
        mutating_calls = set()
        for name in used:
            target = self.classes[dependencies[name].base_type]
            for method in re.findall(rf"\b{re.escape(name)}\s*(?:\.|->)\s*(\w+)\s*\(", body):
                if not target.methods.get(method, False):
                    mutating_calls.add((name, method))

        state_members = {k: v for k, v in class_info.members.items() if k not in dependencies}
        writes = set() if function.is_constructor else written_members(body, state_members)

        raw = {
            "branches": count_decisions(body),
            "parameters": function.parameters,
            "dependencies": len(used),
            "side_effects": len(writes) + len(mutating_calls),
            "error_handling": count_error_handling(body),
            "exceptions": count_exceptions(body),
            "mocking": sum(1 for n in used if self.classes[dependencies[n].base_type].is_interface),
        }
        levels = {factor: to_level(factor, value) for factor, value in raw.items()}
        return Result(function.class_name, function.name, raw, levels)

    def reachable_bodies(self, function: Function, class_info: ClassInfo) -> list[str]:
        """Tijelo funkcije i tijela metoda iste klase koje ona (posredno) poziva."""
        bodies, visited, queue = [], set(), [function]
        while queue:
            current = queue.pop()
            if current.name in visited:
                continue
            visited.add(current.name)
            bodies.append(current.body)
            for name in own_method_calls(current.body, class_info):
                callee = self.functions.get((class_info.name, name))
                if callee:
                    queue.append(callee)
        return bodies


# Nivoi, ocjena i izvjestaj

def to_level(factor: str, value: int) -> int:
    for level, limit in enumerate(THRESHOLDS[factor]):
        if value <= limit:
            return level
    return 3


def difficulty_grade(score: int) -> int:
    # Koristimo istu skalu kao pri rucnom mjerenju kompleksnosti
    if score <= 2:
        return 1
    if score <= 4:
        return 2
    if score <= 6:
        return 3
    if score <= 9:
        return 4
    return 5


def load_manual(path: Path) -> dict[tuple[str, str], int]:
    if not path.exists():
        return {}
    with path.open("r", encoding="utf-8", newline="") as file:
        return {
            (row["class_name"], row["function_name"]): sum(int(row.get(f) or 0) for f in FACTORS)
            for row in csv.DictReader(file)
        }


def create_report(results: list[Result], manual: dict[tuple[str, str], int]) -> str:
    lines = [
        "# Извјештај о комплексности функција",
        "",
        "Фактори (ставка 4 у захтјевима рада) су израчунати аутоматски",
        "из изворног кода (`src/`, `include/`).",
        "За сваки фактор у загради је стварни број, а испред њега ниво 0–3.",
        "",
        "| Класа | Функција | " + " | ".join(FACTOR_NAMES[f] for f in FACTORS)
        + " | Бодови | Оцјена |",
        "|---|---|" + "---:|" * len(FACTORS) + "---:|---:|",
    ]
    for r in sorted(results, key=lambda r: (-r.score, r.class_name, r.function_name)):
        cells = " | ".join(f"{r.levels[f]} ({r.raw[f]})" for f in FACTORS)
        lines.append(
            f"| {r.class_name} | {r.function_name} | {cells} | {r.score} | {r.grade}/5 |"
        )

    compared = [r for r in results if (r.class_name, r.function_name) in manual]
    if compared:
        lines += [
            "",
            "## Поређење са ручном процјеном (`analysis/functions.csv`)",
            "",
            "| Класа | Функција | Ручно (бодови) | Аутоматски (бодови) | Ручна оцјена | Аутоматска оцјена |",
            "|---|---|---:|---:|---:|---:|",
        ]
        for r in compared:
            manual_score = manual[(r.class_name, r.function_name)]
            lines.append(
                f"| {r.class_name} | {r.function_name} | {manual_score} | {r.score} "
                f"| {difficulty_grade(manual_score)}/5 | {r.grade}/5 |"
            )

    lines += [
        "",
        "## Фактори и нивои",
        "",
        "| Фактор | Шта се броји | Ниво 0 | Ниво 1 | Ниво 2 | Ниво 3 |",
        "|---|---|---|---|---|---|",
    ]
    descriptions = {
        "branches": "if, for, while, case, catch, ?:, &&, \\|\\|",
        "parameters": "број параметара",
        "dependencies": "спољне зависности (референца/показивач на класу пројекта)",
        "side_effects": "уписи у чланове класе и позиви не-const метода зависности",
        "error_handling": "return false, return …Invalid/Failed…",
        "exceptions": "throw, catch",
        "mocking": "зависности чији је тип интерфејс",
    }
    for f in FACTORS:
        a, b, c = THRESHOLDS[f]
        lines.append(
            f"| {FACTOR_NAMES[f]} | {descriptions[f]} | {range_text(0, a)} "
            f"| {range_text(a + 1, b)} | {range_text(b + 1, c)} | ≥ {c + 1} |"
        )
    lines += [
        "",
        "Бодови = збир нивоа (0–21). Оцјена: 0–2 → 1, 3–4 → 2, 5–6 → 3, 7–9 → 4, 10+ → 5.",
        "",
        "Гранања, споредни ефекти, обрада грешака и изузеци укључују и методе исте класе",
        "које функција позива, јер и њих тест мора проћи.",
        "",
        "Конструктори: зависности се броје и из иницијализационе листе (нпр. `repo_(repo)`),",
        "а споредни ефекти се за конструкторе не рачунају, јер конструктор иницијализује",
        "чланове, а не мијења их.",
        "",
        "Оцјена је оријентациона и мора бити образложена анализом кода и тестова.",
    ]
    return "\n".join(lines) + "\n"


def range_text(low: int, high: int) -> str:
    return str(low) if low == high else f"{low}–{high}"


def write_csv(results: list[Result], path: Path) -> None:
    with path.open("w", encoding="utf-8", newline="") as file:
        writer = csv.writer(file)
        writer.writerow(["class_name", "function_name", *FACTORS])
        for r in results:
            writer.writerow([r.class_name, r.function_name, *(r.levels[f] for f in FACTORS)])


def analyze_project(root: Path) -> list[Result]:
    classes: dict[str, ClassInfo] = {}
    for header in sorted((root / "include").glob("*.h")):
        for info in parse_header(header.read_text(encoding="utf-8")):
            classes[info.name] = info
    functions: list[Function] = []
    for source in sorted((root / "src").glob("*.cpp")):
        if source.name == "main.cpp":
            continue
        functions += parse_source(source.read_text(encoding="utf-8"))
    return Analyzer(classes, functions).analyze_all()


def main(argv: list[str] | None = None) -> None:
    parser = argparse.ArgumentParser(description="Одређивање комплексности функција")
    parser.add_argument("--root", type=Path, default=PROJECT_ROOT, help="root пројекта")
    parser.add_argument("--output", type=Path, default=OUTPUT_DIRECTORY, help="фолдер за излаз")
    args = parser.parse_args(argv)

    results = analyze_project(args.root)
    manual = load_manual(args.root / "analysis" / "functions.csv")

    args.output.mkdir(parents=True, exist_ok=True)
    report_path = args.output / "complexity_report.md"
    csv_path = args.output / "functions_auto.csv"
    report_path.write_text(create_report(results, manual), encoding="utf-8")
    write_csv(results, csv_path)

    print(f"Анализирано функција: {len(results)}")
    print(f"Извјештај: {report_path}")
    print(f"CSV:       {csv_path}")


if __name__ == "__main__":
    main()