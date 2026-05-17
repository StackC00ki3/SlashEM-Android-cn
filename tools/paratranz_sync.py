#!/usr/bin/env python3
"""Sync untranslated C string literals with Paratranz JSON and apply translations."""

from __future__ import annotations

import argparse
import html
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


ASCII_LETTER_RE = re.compile(r"[A-Za-z]")
CJK_RE = re.compile(r"[\u3400-\u4dbf\u4e00-\u9fff\uf900-\ufaff]")
STRING_PREFIXES = ("u8", "u", "U", "L")
IGNORED_SOURCE_FILES = {
    "monst.c",
    "objects.c",
    "tile.c",
    "version.c",
    "vis_tab.c",
}


@dataclass(frozen=True)
class LiteralEntry:
    original: str
    line: int
    column: int
    context: str
    start_index: int
    end_index: int
    prefix: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Sync untranslated src/*.c strings with Paratranz JSON."
    )
    parser.add_argument(
        "command",
        nargs="?",
        choices=("sync", "apply"),
        default="sync",
        help="Use 'sync' to refresh Paratranz JSON or 'apply' to write translations back.",
    )
    parser.add_argument(
        "--src-dir",
        default="src",
        help="Directory containing .c files to scan.",
    )
    parser.add_argument(
        "--output-dir",
        default="paratranz/src",
        help="Directory containing per-file Paratranz JSON.",
    )
    parser.add_argument(
        "--window",
        type=int,
        default=5,
        help="Context line window above and below the source line.",
    )
    return parser.parse_args()


def contains_ascii_letters(text: str) -> bool:
    return bool(ASCII_LETTER_RE.search(text))


def contains_cjk(text: str) -> bool:
    return bool(CJK_RE.search(text))


def is_translatable(text: str) -> bool:
    return contains_ascii_letters(text) and not contains_cjk(text)


def normalize_generated_text(text: str) -> str:
    return html.unescape(text)


def build_line_starts(source: str) -> list[int]:
    starts = [0]
    for idx, char in enumerate(source):
        if char == "\n":
            starts.append(idx + 1)
    return starts


def index_to_line_col(line_starts: list[int], index: int) -> tuple[int, int]:
    lo = 0
    hi = len(line_starts)
    while lo + 1 < hi:
        mid = (lo + hi) // 2
        if line_starts[mid] <= index:
            lo = mid
        else:
            hi = mid
    line_start = line_starts[lo]
    return lo + 1, index - line_start + 1


def detect_preprocessor_lines(lines: list[str]) -> set[int]:
    marked: set[int] = set()
    in_directive = False
    for lineno, line in enumerate(lines, start=1):
        if in_directive:
            marked.add(lineno)
        else:
            stripped = line.lstrip()
            if stripped.startswith("#"):
                marked.add(lineno)
                in_directive = True
        if in_directive:
            stripped_newline = line.rstrip("\n")
            backslashes = 0
            for char in reversed(stripped_newline):
                if char == "\\":
                    backslashes += 1
                else:
                    break
            in_directive = bool(backslashes % 2)
    return marked


def skip_ws_and_comments(
    source: str, index: int, preprocessor_lines: set[int], line_starts: list[int]
) -> int:
    length = len(source)
    while index < length:
        line, _ = index_to_line_col(line_starts, index)
        if line in preprocessor_lines:
            line_end = source.find("\n", index)
            return length if line_end == -1 else line_end + 1

        if source[index].isspace():
            index += 1
            continue
        if source.startswith("//", index):
            line_end = source.find("\n", index)
            index = length if line_end == -1 else line_end + 1
            continue
        if source.startswith("/*", index):
            comment_end = source.find("*/", index + 2)
            index = length if comment_end == -1 else comment_end + 2
            continue
        break
    return index


def match_string_prefix(source: str, index: int) -> tuple[str, int] | None:
    if source[index] == '"':
        return "", index
    for prefix in STRING_PREFIXES:
        end = index + len(prefix)
        if source.startswith(prefix, index) and end < len(source) and source[end] == '"':
            return prefix, end
    return None


def parse_string_token(source: str, quote_index: int) -> tuple[str, int]:
    index = quote_index + 1
    chunks: list[str] = []
    while index < len(source):
        char = source[index]
        if char == "\\":
            if index + 1 < len(source):
                chunks.append(source[index : index + 2])
                index += 2
            else:
                chunks.append(char)
                index += 1
            continue
        if char == '"':
            return "".join(chunks), index + 1
        if char == "\n":
            raise ValueError(f"Unterminated string literal near byte offset {quote_index}")
        chunks.append(char)
        index += 1
    raise ValueError(f"Unterminated string literal near byte offset {quote_index}")


def collect_literals(path: Path, window: int) -> list[LiteralEntry]:
    source = path.read_text(encoding="utf-8", errors="surrogateescape")
    return collect_literals_from_text(path, source, window)


def collect_literals_from_text(path: Path, source: str, window: int) -> list[LiteralEntry]:
    lines = source.splitlines(keepends=True)
    line_starts = build_line_starts(source)
    preprocessor_lines = detect_preprocessor_lines(lines)
    entries: list[LiteralEntry] = []
    index = 0
    length = len(source)

    while index < length:
        line, _ = index_to_line_col(line_starts, index)
        if line in preprocessor_lines:
            line_end = source.find("\n", index)
            index = length if line_end == -1 else line_end + 1
            continue

        if source.startswith("//", index):
            line_end = source.find("\n", index)
            index = length if line_end == -1 else line_end + 1
            continue
        if source.startswith("/*", index):
            comment_end = source.find("*/", index + 2)
            index = length if comment_end == -1 else comment_end + 2
            continue
        if source[index] == "'":
            index += 1
            while index < length:
                if source[index] == "\\" and index + 1 < length:
                    index += 2
                elif source[index] == "'":
                    index += 1
                    break
                else:
                    index += 1
            continue

        matched = match_string_prefix(source, index)
        if not matched:
            index += 1
            continue

        prefix, _ = matched
        start_line, start_col = index_to_line_col(line_starts, index)
        parts: list[str] = []
        current_index = index

        malformed = False
        while True:
            matched_current = match_string_prefix(source, current_index)
            if not matched_current:
                malformed = True
                current_index += 1
                break
            _, current_quote = matched_current
            try:
                part, after = parse_string_token(source, current_quote)
            except ValueError:
                current_index = current_quote + 1
                malformed = True
                break
            parts.append(part)
            next_index = skip_ws_and_comments(source, after, preprocessor_lines, line_starts)
            if next_index >= length:
                current_index = next_index
                break
            next_line, _ = index_to_line_col(line_starts, next_index)
            if next_line in preprocessor_lines:
                current_index = next_index
                break
            if match_string_prefix(source, next_index):
                current_index = next_index
                continue
            current_index = next_index
            break

        if malformed:
            index = current_index
            continue

        original = normalize_generated_text("".join(parts))
        if is_translatable(original):
            context = build_context(path, lines, start_line, start_col, window)
            entries.append(
                LiteralEntry(
                    original=original,
                    line=start_line,
                    column=start_col,
                    context=context,
                    start_index=index,
                    end_index=current_index,
                    prefix=prefix,
                )
            )
        index = current_index

    return entries


def build_context(
    path: Path, lines: list[str], line_number: int, column_number: int, window: int
) -> str:
    start = max(1, line_number - window)
    end = min(len(lines), line_number + window)
    context_lines = [f"{path.as_posix()}:{line_number}:{column_number}"]
    for lineno in range(start, end + 1):
        prefix = "TARGET" if lineno == line_number else "      "
        text = normalize_generated_text(lines[lineno - 1].rstrip("\n"))
        context_lines.append(f"{prefix}{lineno:5d}: {text}")
    return "\n".join(context_lines)


def load_existing_records(path: Path) -> list[dict]:
    if not path.exists():
        return []
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError:
        return []
    if isinstance(data, list):
        return [item for item in data if isinstance(item, dict)]
    return []


def assign_keys(entries: list[LiteralEntry]) -> list[tuple[str, LiteralEntry]]:
    keyed_entries: list[tuple[str, LiteralEntry]] = []
    seen_per_line: dict[int, int] = {}
    for entry in entries:
        seen_per_line[entry.line] = seen_per_line.get(entry.line, 0) + 1
        occurrence_on_line = seen_per_line[entry.line]
        key = f"L{entry.line:05d}_{occurrence_on_line:02d}"
        keyed_entries.append((key, entry))
    return keyed_entries


def assign_translations(entries: list[LiteralEntry], existing: list[dict]) -> list[dict]:
    records: list[dict] = []
    old_by_key = {
        item.get("key"): item
        for item in existing
        if isinstance(item.get("key"), str)
    }

    old_by_original: dict[str, list[dict]] = {}
    for item in existing:
        original = item.get("original")
        if isinstance(original, str):
            old_by_original.setdefault(original, []).append(item)

    seen_per_original: dict[str, int] = {}

    for key, entry in assign_keys(entries):
        translation = ""
        old = old_by_key.get(key)
        if isinstance(old, dict):
            translation = str(old.get("translation", ""))
        else:
            seen_per_original[entry.original] = seen_per_original.get(entry.original, 0) + 1
            occurrence = seen_per_original[entry.original]
            matches = old_by_original.get(entry.original, [])
            if occurrence <= len(matches):
                translation = str(matches[occurrence - 1].get("translation", ""))

        records.append(
            {
                "key": key,
                "original": entry.original,
                "translation": translation,
                "context": entry.context,
            }
        )
    return records


def write_json(path: Path, records: list[dict]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(
        json.dumps(records, ensure_ascii=False, indent=2) + "\n",
        encoding="utf-8",
    )


def iter_stale_json_files(output_dir: Path, expected_paths: Iterable[Path]) -> Iterable[Path]:
    expected = {path.resolve() for path in expected_paths}
    if not output_dir.exists():
        return []
    stale: list[Path] = []
    for json_file in output_dir.rglob("*.json"):
        if json_file.resolve() not in expected:
            stale.append(json_file)
    return stale


def escape_c_string(text: str) -> str:
    escaped: list[str] = []
    for char in text:
        if char == "\\":
            escaped.append("\\\\")
        elif char == '"':
            escaped.append('\\"')
        elif char == "\n":
            escaped.append("\\n")
        elif char == "\r":
            escaped.append("\\r")
        elif char == "\t":
            escaped.append("\\t")
        elif char == "\0":
            escaped.append("\\0")
        else:
            escaped.append(char)
    return "".join(escaped)


def iter_source_files(src_dir: Path) -> list[Path]:
    return sorted(
        path for path in src_dir.glob("*.c") if path.name not in IGNORED_SOURCE_FILES
    )


def apply_translations_to_file(source_file: Path, json_file: Path, window: int) -> bool:
    if not json_file.exists():
        return False

    records = load_existing_records(json_file)
    if not records:
        return False

    source = source_file.read_text(encoding="utf-8", errors="surrogateescape")
    entries = collect_literals_from_text(source_file, source, window)
    translations_by_key = {
        item["key"]: str(item.get("translation", ""))
        for item in records
        if isinstance(item.get("key"), str)
    }

    replacements: list[tuple[int, int, str]] = []
    for key, entry in assign_keys(entries):
        translation = translations_by_key.get(key, "")
        if not translation.strip():
            continue
        replacement = f'{entry.prefix}"{escape_c_string(translation)}"'
        replacements.append((entry.start_index, entry.end_index, replacement))

    if not replacements:
        return False

    parts: list[str] = []
    cursor = 0
    for start_index, end_index, replacement in replacements:
        parts.append(source[cursor:start_index])
        parts.append(replacement)
        cursor = end_index
    parts.append(source[cursor:])
    updated_source = "".join(parts)

    if updated_source == source:
        return False

    source_file.write_text(updated_source, encoding="utf-8")
    return True


def sync_paratranz(src_dir: Path, output_dir: Path, window: int) -> int:
    source_files = iter_source_files(src_dir)
    expected_outputs: list[Path] = []

    for source_file in source_files:
        entries = collect_literals(source_file, window)
        output_file = output_dir / f"{source_file.name}.json"
        records = assign_translations(entries, load_existing_records(output_file))
        write_json(output_file, records)
        expected_outputs.append(output_file)

    for stale_file in iter_stale_json_files(output_dir, expected_outputs):
        stale_file.unlink()

    return 0


def apply_paratranz(src_dir: Path, output_dir: Path, window: int) -> int:
    for source_file in iter_source_files(src_dir):
        json_file = output_dir / f"{source_file.name}.json"
        apply_translations_to_file(source_file, json_file, window)
    return 0


def main() -> int:
    args = parse_args()
    src_dir = Path(args.src_dir)
    output_dir = Path(args.output_dir)

    if args.command == "apply":
        return apply_paratranz(src_dir, output_dir, args.window)
    return sync_paratranz(src_dir, output_dir, args.window)


if __name__ == "__main__":
    raise SystemExit(main())
