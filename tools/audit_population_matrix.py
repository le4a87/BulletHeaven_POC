#!/usr/bin/env python3
"""Audit BH-009 fixed-cap telemetry coverage.

This helper scans telemetry CSVs from Saved/Profiling and reports which formal
fixed-cap BH-009 rows have usable captures. A usable capture has exactly one
active cap row and reaches a configurable fraction of that cap as live enemies.
"""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass
from pathlib import Path

from compare_population_telemetry import CapSummary, load_csv


DEFAULT_REQUIRED_CAPS = (25, 50, 75, 100)
DEFAULT_WORKLOAD_RATIO = 0.90


@dataclass
class FixedCapCandidate:
    path: Path
    summary: CapSummary


def find_fixed_cap_candidates(
    telemetry_dir: Path,
    ignore_before_seconds: float,
    workload_ratio: float,
) -> dict[int, list[FixedCapCandidate]]:
    candidates_by_cap: dict[int, list[FixedCapCandidate]] = {}
    for path in sorted(telemetry_dir.glob("BH009_*.csv")):
        summaries = load_csv(path, ignore_before_seconds)
        if len(summaries) != 1:
            continue

        cap, summary = next(iter(summaries.items()))
        min_live = int(cap * workload_ratio)
        if summary.peak_live_enemies < min_live:
            continue

        candidates_by_cap.setdefault(cap, []).append(FixedCapCandidate(path=path, summary=summary))

    return candidates_by_cap


def select_best_candidate(candidates: list[FixedCapCandidate]) -> FixedCapCandidate:
    return max(
        candidates,
        key=lambda candidate: (
            candidate.summary.samples,
            candidate.summary.peak_live_enemies,
            candidate.path.stat().st_mtime,
        ),
    )


def render_audit(
    required_caps: list[int],
    candidates_by_cap: dict[int, list[FixedCapCandidate]],
) -> tuple[str, bool]:
    lines = [
        "| Cap | Status | Telemetry CSV | Samples | Window | Peak Live | p95 Avg Frame | Max Avg Frame | Peak Physical MB |",
        "| ---: | --- | --- | ---: | --- | ---: | ---: | ---: | ---: |",
    ]
    complete = True
    for cap in required_caps:
        candidates = candidates_by_cap.get(cap, [])
        if not candidates:
            complete = False
            lines.append(f"| {cap} | Missing |  |  |  |  |  |  |  |")
            continue

        best = select_best_candidate(candidates)
        summary = best.summary
        lines.append(
            f"| {cap} | Present | `{best.path}` | {summary.samples} "
            f"| {summary.start_seconds:.1f}-{summary.end_seconds:.1f}s "
            f"| {summary.peak_live_enemies} | {summary.p95_avg_frame_ms:.2f} ms "
            f"| {summary.max_avg_frame_ms:.2f} ms | {summary.peak_used_physical_mb:.1f} |"
        )

    return "\n".join(lines), complete


def build_audit_rows(
    required_caps: list[int],
    candidates_by_cap: dict[int, list[FixedCapCandidate]],
) -> tuple[list[dict[str, object]], list[int]]:
    rows: list[dict[str, object]] = []
    missing_caps: list[int] = []
    for cap in required_caps:
        candidates = candidates_by_cap.get(cap, [])
        if not candidates:
            missing_caps.append(cap)
            rows.append({"cap": cap, "status": "missing"})
            continue

        best = select_best_candidate(candidates)
        rows.append(
            {
                "cap": cap,
                "status": "present",
                "telemetry_csv": str(best.path),
                "summary": asdict(best.summary),
            }
        )

    return rows, missing_caps


def render_next_cap_instructions(next_cap: int) -> str:
    return "\n".join(
        [
            f"Next missing BH-009 fixed-cap trial: {next_cap}",
            "",
            "Set these console variables before starting PIE:",
            "",
            "```text",
            "bh.SpawnPressure.Enabled 0",
            f"bh.PopulationProfile.FixedCap {next_cap}",
            "bh.EnemyBudget.Enabled 1",
            "```",
            "",
            "After stopping PIE, run:",
            "",
            "```bash",
            "python3 tools/audit_population_matrix.py",
            "```",
        ]
    )


def parse_caps(value: str) -> list[int]:
    caps = [int(item.strip()) for item in value.split(",") if item.strip()]
    if not caps:
        raise argparse.ArgumentTypeError("at least one cap is required")
    return caps


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--telemetry-dir", type=Path, default=Path("Saved/Profiling"))
    parser.add_argument("--caps", type=parse_caps, default=list(DEFAULT_REQUIRED_CAPS))
    parser.add_argument("--ignore-before-seconds", type=float, default=3.0)
    parser.add_argument("--workload-ratio", type=float, default=DEFAULT_WORKLOAD_RATIO)
    parser.add_argument("--json", type=Path, help="Optional path to write audit JSON")
    parser.add_argument("--next", action="store_true", help="Print the next missing fixed-cap PIE setup and exit 0")
    args = parser.parse_args()

    candidates_by_cap = find_fixed_cap_candidates(
        args.telemetry_dir,
        args.ignore_before_seconds,
        args.workload_ratio,
    )
    rows, missing_caps = build_audit_rows(args.caps, candidates_by_cap)

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(
            json.dumps(
                {
                    "required_caps": args.caps,
                    "missing_caps": missing_caps,
                    "complete": not missing_caps,
                    "rows": rows,
                },
                indent=2,
            )
            + "\n"
        )

    if args.next:
        if not missing_caps:
            print("All required BH-009 fixed-cap telemetry rows are present.")
            return 0

        print(render_next_cap_instructions(missing_caps[0]))
        return 0

    table, complete = render_audit(args.caps, candidates_by_cap)
    print(table)

    if complete:
        print("\nResult: COMPLETE, all required fixed-cap telemetry rows are present.")
        return 0

    print("\nResult: INCOMPLETE, one or more required fixed-cap telemetry rows are missing.")
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
