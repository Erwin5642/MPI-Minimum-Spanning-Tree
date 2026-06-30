#!/usr/bin/env python3
"""Check whether caminho.txt lines are correctly ordered by edge weight."""

import argparse
import sys
from decimal import Decimal, InvalidOperation
from pathlib import Path


def parse_line(line: str, line_no: int) -> tuple[Decimal, int, int, str]:
    parts = line.split()
    if len(parts) < 3:
        raise ValueError(f"expected 'source weight destination', got: {line!r}")

    try:
        source = int(parts[0])
        weight = Decimal(parts[1])
        destination = int(parts[2])
    except (ValueError, InvalidOperation) as exc:
        raise ValueError(f"invalid numeric field: {exc}") from exc

    return weight, source, destination, parts[1]


def check_weight_order(path: Path, strict: bool) -> int:
    edge_count = 0
    violations = 0
    first_violation = None

    prev_weight: Decimal | None = None
    prev_source: int | None = None
    prev_destination: int | None = None
    prev_weight_text: str | None = None
    prev_line_no: int | None = None

    with path.open(encoding="utf-8") as file:
        for line_no, line in enumerate(file, start=1):
            line = line.strip()
            if not line:
                continue

            try:
                weight, source, destination, weight_text = parse_line(line, line_no)
            except ValueError as exc:
                print(f"error: {path}:{line_no}: {exc}", file=sys.stderr)
                return 1

            edge_count += 1

            if prev_weight is None:
                prev_weight = weight
                prev_source = source
                prev_destination = destination
                prev_weight_text = weight_text
                prev_line_no = line_no
                continue

            ordered = True
            if weight < prev_weight:
                ordered = False
            elif strict and weight == prev_weight:
                if (source, destination) < (prev_source, prev_destination):
                    ordered = False

            if not ordered:
                violations += 1
                if first_violation is None:
                    first_violation = {
                        "line_no": line_no,
                        "prev_line_no": prev_line_no,
                        "prev": (prev_source, prev_weight_text, prev_destination),
                        "current": (source, weight_text, destination),
                    }

            prev_weight = weight
            prev_source = source
            prev_destination = destination
            prev_weight_text = weight_text
            prev_line_no = line_no

    print(f"file: {path}")
    print(f"edges: {edge_count}")
    print(f"order rule: weight ascending{' + (source, destination) on ties' if strict else ''}")

    if violations == 0:
        print("result: correctly ordered")
        return 0

    print(f"result: not correctly ordered ({violations} violation(s))")
    if first_violation:
        prev = first_violation["prev"]
        current = first_violation["current"]
        print(
            "first violation:"
            f" line {first_violation['prev_line_no']} -> {first_violation['line_no']}"
        )
        print(f"  previous:  {prev[0]} {prev[1]} {prev[2]}")
        print(f"  current:   {current[0]} {current[1]} {current[2]}")

    return 1


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Check if caminho.txt is ordered by edge weight."
    )
    parser.add_argument(
        "file",
        nargs="?",
        default="caminho.txt",
        type=Path,
        help="input file (default: caminho.txt)",
    )
    parser.add_argument(
        "--strict",
        action="store_true",
        help="also require ascending (source, destination) when weights tie",
    )
    args = parser.parse_args()

    if not args.file.is_file():
        print(f"error: file not found: {args.file}", file=sys.stderr)
        return 1

    return check_weight_order(args.file, args.strict)


if __name__ == "__main__":
    raise SystemExit(main())
