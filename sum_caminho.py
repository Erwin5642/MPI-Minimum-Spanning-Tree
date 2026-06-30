#!/usr/bin/env python3
"""Sum MST edge weights from a caminho-style text file using double and Kahan summation."""

import argparse
import sys
from pathlib import Path


def parse_weight(line: str) -> float:
    parts = line.split()
    if len(parts) < 3:
        raise ValueError(f"expected 'source weight destination', got: {line!r}")
    return float(parts[1])


def double_sum(weights) -> float:
    total = 0.0
    for w in weights:
        total += w
    return total


def kahan_sum(weights) -> float:
    total = 0.0
    compensation = 0.0
    for w in weights:
        y = w - compensation
        t = total + y
        compensation = (t - total) - y
        total = t
    return total


def iter_weights(path: Path):
    with path.open(encoding="utf-8") as f:
        for line_no, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                yield parse_weight(line)
            except ValueError as exc:
                raise ValueError(f"{path}:{line_no}: {exc}") from exc


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Sum edge weights from a caminho.txt file."
    )
    parser.add_argument(
        "file",
        nargs="?",
        default="caminho.txt",
        type=Path,
        help="input file (default: caminho.txt)",
    )
    args = parser.parse_args()

    if not args.file.is_file():
        print(f"error: file not found: {args.file}", file=sys.stderr)
        return 1

    weights = list(iter_weights(args.file))
    edge_count = len(weights)

    naive = double_sum(weights)
    kahan = kahan_sum(weights)

    print(f"edges: {edge_count}")
    print(f"double sum: {naive:.17g}")
    print(f"kahan sum:  {kahan:.17g}")
    print(f"difference: {kahan - naive:.17g}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
