#!/usr/bin/env python3
"""Compare Bullet Heaven population telemetry CSV files.

The telemetry CSVs are written by UBHPopulationTelemetrySubsystem under
Saved/Profiling/BH009_*.csv. This tool summarizes frame-time, population, and
memory metrics per active spawner cap, then optionally compares a candidate run
against a baseline run.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from statistics import mean


DEFAULT_FRAME_REGRESSION_RATIO = 1.15
DEFAULT_FRAME_REGRESSION_MS = 2.0
DEFAULT_MEMORY_REGRESSION_MB = 256.0
DEFAULT_WORKLOAD_FLOOR_RATIO = 0.90
DEFAULT_BASELINE_MANIFEST = Path("tools/telemetry_baselines.json")


@dataclass
class CapSummary:
    cap: int
    samples: int
    start_seconds: float
    end_seconds: float
    avg_fps: float
    min_fps: float
    avg_frame_ms: float
    p50_avg_frame_ms: float
    p95_avg_frame_ms: float
    p99_avg_frame_ms: float
    max_avg_frame_ms: float
    max_worst_frame_ms: float
    peak_live_enemies: int
    peak_projectiles: int
    obstacle_count: int
    peak_used_physical_mb: float
    peak_used_virtual_mb: float
    budget_enabled_samples: int
    peak_budget_near: int
    peak_budget_mid: int
    peak_budget_far: int
    peak_anim_rate_varied: int


def percentile(sorted_values: list[float], fraction: float) -> float:
    if not sorted_values:
        return 0.0
    index = math.ceil(len(sorted_values) * fraction) - 1
    index = max(0, min(index, len(sorted_values) - 1))
    return sorted_values[index]


def read_float(row: dict[str, str], field: str, default: float = 0.0) -> float:
    value = row.get(field, "")
    try:
        return float(value)
    except ValueError:
        return default


def read_int(row: dict[str, str], field: str, default: int = 0) -> int:
    return int(round(read_float(row, field, float(default))))


def load_csv(path: Path, ignore_before_seconds: float) -> dict[int, CapSummary]:
    samples_by_cap: dict[int, list[dict[str, str]]] = {}
    with path.open(newline="") as handle:
        reader = csv.DictReader(handle)
        required = {
            "ElapsedSeconds",
            "AvgFrameMs",
            "WorstFrameMs",
            "AvgFPS",
            "LiveEnemies",
            "Projectiles",
            "Obstacles",
            "SpawnerMaxEnemiesAlive",
            "UsedPhysicalMB",
            "UsedVirtualMB",
        }
        missing = required.difference(reader.fieldnames or [])
        if missing:
            raise ValueError(f"{path} is missing required columns: {', '.join(sorted(missing))}")

        for row in reader:
            elapsed = read_float(row, "ElapsedSeconds")
            if elapsed < ignore_before_seconds:
                continue
            cap = read_int(row, "SpawnerMaxEnemiesAlive")
            samples_by_cap.setdefault(cap, []).append(row)

    summaries: dict[int, CapSummary] = {}
    for cap, rows in samples_by_cap.items():
        avg_frames = sorted(read_float(row, "AvgFrameMs") for row in rows)
        fps_values = [read_float(row, "AvgFPS") for row in rows]
        elapsed_values = [read_float(row, "ElapsedSeconds") for row in rows]
        summaries[cap] = CapSummary(
            cap=cap,
            samples=len(rows),
            start_seconds=min(elapsed_values),
            end_seconds=max(elapsed_values),
            avg_fps=mean(fps_values),
            min_fps=min(fps_values),
            avg_frame_ms=mean(avg_frames),
            p50_avg_frame_ms=percentile(avg_frames, 0.50),
            p95_avg_frame_ms=percentile(avg_frames, 0.95),
            p99_avg_frame_ms=percentile(avg_frames, 0.99),
            max_avg_frame_ms=max(avg_frames),
            max_worst_frame_ms=max(read_float(row, "WorstFrameMs") for row in rows),
            peak_live_enemies=max(read_int(row, "LiveEnemies") for row in rows),
            peak_projectiles=max(read_int(row, "Projectiles") for row in rows),
            obstacle_count=max(read_int(row, "Obstacles") for row in rows),
            peak_used_physical_mb=max(read_float(row, "UsedPhysicalMB") for row in rows),
            peak_used_virtual_mb=max(read_float(row, "UsedVirtualMB") for row in rows),
            budget_enabled_samples=sum(1 for row in rows if read_int(row, "EnemyBudgetEnabled") > 0),
            peak_budget_near=max(read_int(row, "EnemyBudgetNear") for row in rows),
            peak_budget_mid=max(read_int(row, "EnemyBudgetMid") for row in rows),
            peak_budget_far=max(read_int(row, "EnemyBudgetFar") for row in rows),
            peak_anim_rate_varied=max(read_int(row, "EnemyAnimRateVaried") for row in rows),
        )
    return summaries


def resolve_latest_candidate(search_dir: Path) -> Path:
    candidates = sorted(search_dir.glob("BH009_*.csv"), key=lambda item: item.stat().st_mtime, reverse=True)
    if not candidates:
        raise FileNotFoundError(f"no BH009_*.csv files found in {search_dir}")
    return candidates[0]


def load_baseline_profile(manifest_path: Path, profile_name: str) -> Path:
    with manifest_path.open() as handle:
        manifest = json.load(handle)

    profiles = manifest.get("profiles", {})
    if profile_name not in profiles:
        available = ", ".join(sorted(profiles)) or "none"
        raise KeyError(f"baseline profile '{profile_name}' not found in {manifest_path}; available: {available}")

    profile = profiles[profile_name]
    baseline = profile.get("baseline")
    if not baseline:
        raise ValueError(f"baseline profile '{profile_name}' does not define a baseline path")

    return Path(baseline)


def render_summary(summaries: dict[int, CapSummary]) -> str:
    lines = [
        "| Cap | Samples | Window | Avg FPS | p95 Avg Frame | Max Avg Frame | Worst Frame | Peak Live | Peak Projectiles | Peak Physical MB | Budget Samples | Peak Budget Bands | Peak Anim Varied |",
        "| ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |",
    ]
    for cap in sorted(summaries):
        item = summaries[cap]
        lines.append(
            f"| {cap} | {item.samples} | {item.start_seconds:.1f}-{item.end_seconds:.1f}s "
            f"| {item.avg_fps:.1f} | {item.p95_avg_frame_ms:.2f} ms "
            f"| {item.max_avg_frame_ms:.2f} ms | {item.max_worst_frame_ms:.2f} ms "
            f"| {item.peak_live_enemies} | {item.peak_projectiles} | {item.peak_used_physical_mb:.1f} "
            f"| {item.budget_enabled_samples} | {item.peak_budget_near}/{item.peak_budget_mid}/{item.peak_budget_far} "
            f"| {item.peak_anim_rate_varied} |"
        )
    return "\n".join(lines)


def compare(
    baseline: dict[int, CapSummary],
    candidate: dict[int, CapSummary],
    frame_ratio: float,
    frame_delta_ms: float,
    memory_delta_mb: float,
    workload_floor_ratio: float,
) -> tuple[list[str], list[str]]:
    failures: list[str] = []
    warnings: list[str] = []

    for cap in sorted(baseline):
        if cap not in candidate:
            failures.append(f"cap {cap}: missing candidate data")
            continue

        base = baseline[cap]
        cand = candidate[cap]
        min_live = math.floor(base.peak_live_enemies * workload_floor_ratio)
        if cand.peak_live_enemies < min_live:
            failures.append(
                f"cap {cap}: candidate peak live enemies {cand.peak_live_enemies} "
                f"is below comparable workload floor {min_live}"
            )

        allowed_p95 = max(base.p95_avg_frame_ms * frame_ratio, base.p95_avg_frame_ms + frame_delta_ms)
        if cand.p95_avg_frame_ms > allowed_p95:
            failures.append(
                f"cap {cap}: p95 AvgFrameMs regressed from {base.p95_avg_frame_ms:.2f} "
                f"to {cand.p95_avg_frame_ms:.2f} ms (allowed {allowed_p95:.2f})"
            )

        allowed_max_avg = max(base.max_avg_frame_ms * frame_ratio, base.max_avg_frame_ms + frame_delta_ms)
        if cand.max_avg_frame_ms > allowed_max_avg:
            warnings.append(
                f"cap {cap}: max AvgFrameMs rose from {base.max_avg_frame_ms:.2f} "
                f"to {cand.max_avg_frame_ms:.2f} ms (warning threshold {allowed_max_avg:.2f})"
            )

        allowed_memory = base.peak_used_physical_mb + memory_delta_mb
        if cand.peak_used_physical_mb > allowed_memory:
            failures.append(
                f"cap {cap}: peak physical memory rose from {base.peak_used_physical_mb:.1f} "
                f"to {cand.peak_used_physical_mb:.1f} MB (allowed {allowed_memory:.1f})"
            )

    extra_caps = sorted(set(candidate).difference(baseline))
    for cap in extra_caps:
        warnings.append(f"cap {cap}: candidate has no baseline; summarized but not regression-checked")

    return failures, warnings


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline", type=Path, help="Baseline BH009 telemetry CSV")
    parser.add_argument("--baseline-profile", help="Named baseline from tools/telemetry_baselines.json")
    parser.add_argument("--baseline-manifest", type=Path, default=DEFAULT_BASELINE_MANIFEST)
    parser.add_argument("--candidate", type=Path, help="Candidate BH009 telemetry CSV")
    parser.add_argument("--latest", action="store_true", help="Use newest BH009_*.csv from --latest-dir as candidate")
    parser.add_argument("--latest-dir", type=Path, default=Path("Saved/Profiling"))
    parser.add_argument("--json", type=Path, help="Optional path to write candidate summary JSON")
    parser.add_argument("--ignore-before-seconds", type=float, default=3.0)
    parser.add_argument("--frame-regression-ratio", type=float, default=DEFAULT_FRAME_REGRESSION_RATIO)
    parser.add_argument("--frame-regression-ms", type=float, default=DEFAULT_FRAME_REGRESSION_MS)
    parser.add_argument("--memory-regression-mb", type=float, default=DEFAULT_MEMORY_REGRESSION_MB)
    parser.add_argument("--workload-floor-ratio", type=float, default=DEFAULT_WORKLOAD_FLOOR_RATIO)
    args = parser.parse_args()

    if args.baseline and args.baseline_profile:
        parser.error("use either --baseline or --baseline-profile, not both")

    if args.candidate and args.latest:
        parser.error("use either --candidate or --latest, not both")

    if args.baseline_profile:
        args.baseline = load_baseline_profile(args.baseline_manifest, args.baseline_profile)

    if args.latest:
        args.candidate = resolve_latest_candidate(args.latest_dir)

    if not args.candidate:
        parser.error("one of --candidate or --latest is required")

    candidate = load_csv(args.candidate, args.ignore_before_seconds)
    print(f"Candidate: {args.candidate}")
    print(render_summary(candidate))

    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps({str(cap): asdict(item) for cap, item in sorted(candidate.items())}, indent=2) + "\n")

    if not args.baseline:
        return 0

    baseline = load_csv(args.baseline, args.ignore_before_seconds)
    print(f"\nBaseline: {args.baseline}")
    print(render_summary(baseline))

    failures, warnings = compare(
        baseline,
        candidate,
        args.frame_regression_ratio,
        args.frame_regression_ms,
        args.memory_regression_mb,
        args.workload_floor_ratio,
    )

    if warnings:
        print("\nWarnings:")
        for warning in warnings:
            print(f"- {warning}")

    if failures:
        print("\nFailures:")
        for failure in failures:
            print(f"- {failure}")
        return 1

    print("\nResult: PASS, no telemetry regressions exceeded thresholds.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
