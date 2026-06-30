#!/usr/bin/env python3
"""Sum MST edge weights using Python Decimal arbitrary-precision arithmetic."""

import argparse
import sys
from decimal import Decimal, getcontext
from pathlib import Path


def parse_weight_text(line: str) -> str:
    parts = line.split()
    if len(parts) < 3:
        raise ValueError(f"expected 'source weight destination', got: {line!r}")
    return parts[1]


def decimal_sum(weights: list[str]) -> Decimal:
    total = Decimal(0)
    for w in weights:
        total += Decimal(w)
    return total


def iter_weight_texts(path: Path):
    with path.open(encoding="utf-8") as f:
        for line_no, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                yield parse_weight_text(line)
            except ValueError as exc:
                raise ValueError(f"{path}:{line_no}: {exc}") from exc


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Sum edge weights from a caminho.txt file using Decimal."
    )
    parser.add_argument(
        "file",
        nargs="?",
        default="caminho.txt",
        type=Path,
        help="input file (default: caminho.txt)",
    )
    parser.add_argument(
        "--precision",
        type=int,
        default=100,
        help="Decimal context precision (default: 100)",
    )
    args = parser.parse_args()

    if not args.file.is_file():
        print(f"error: file not found: {args.file}", file=sys.stderr)
        return 1

    getcontext().prec = args.precision

    weights = list(iter_weight_texts(args.file))
    total = decimal_sum(weights)

    print(f"edges: {len(weights)}")
    print(f"precision: {args.precision}")
    print(f"decimal sum: {total}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
