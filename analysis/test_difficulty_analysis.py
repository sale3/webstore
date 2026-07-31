import csv
from pathlib import Path


DIRECTORY = Path(__file__).resolve().parent
INPUT_FILE = DIRECTORY / "functions.csv"
OUTPUT_FILE = DIRECTORY / "report.md"


def difficulty_grade(score: int) -> int:
    if score <= 2:
        return 1
    if score <= 4:
        return 2
    if score <= 6:
        return 3
    if score <= 9:
        return 4
    return 5


def calculate_score(row: dict[str, str]) -> int:
    fields = [
        "branches",
        "parameters",
        "dependencies",
        "side_effects",
        "error_handling",
        "mocking",
    ]

    return sum(int(row[field]) for field in fields)


def create_report(rows: list[dict[str, str]]) -> str:
    lines = [
        "# Извјештај о тежини тестирања",
        "",
        "| Класа | Функција | Бодови | Оцјена |",
        "|---|---|---:|---:|",
    ]

    for row in rows:
        score = calculate_score(row)
        grade = difficulty_grade(score)

        lines.append(
            f"| {row['class_name']} "
            f"| {row['function_name']} "
            f"| {score} | {grade}/5 |"
        )

    lines.extend([
        "",
        "Оцјена је оријентациона и мора бити "
        "образложена анализом кода и тестова.",
    ])

    return "\n".join(lines)


def main() -> None:
    with INPUT_FILE.open(
        "r",
        encoding="utf-8",
        newline=""
    ) as file:
        rows = list(csv.DictReader(file))

    report = create_report(rows)
    OUTPUT_FILE.write_text(report, encoding="utf-8")

    print(f"Извјештај је креиран: {OUTPUT_FILE}")


if __name__ == "__main__":
    main()
