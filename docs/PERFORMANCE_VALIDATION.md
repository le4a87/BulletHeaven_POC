# Performance Validation

Use this suite after completing gameplay, AI, presentation, or map tasks that could affect runtime cost. It compares `BHPopulationTelemetrySubsystem` CSV files from `Saved/Profiling` and fails when a candidate run is materially worse than a baseline run.

Newer telemetry files may include task-specific diagnostic columns after the core performance fields. The comparison tool keeps older baselines readable and summarizes optional `BH-012` enemy-budget columns when present.

## Capture A Trial

1. Build `BulletHeavenPOCEditor` if native code changed.
2. Restart Unreal Editor so PIE uses the current module.
3. Open `Content/Maps/Map_BulletHeavenPOC`.
4. For fixed-cap `BH-009` trials, set console variable `bh.SpawnPressure.Enabled 0` and `bh.PopulationProfile.FixedCap <cap>` before PIE starts. For ramp-regression trials, leave spawn pressure at `1` and fixed cap at `0`.
5. Run a fresh PIE session long enough to reach the target cap or spawn-pressure tier.
6. Stop PIE and locate the newest telemetry file:

```bash
find Saved/Profiling -maxdepth 1 -type f -name 'BH009_*.csv' -printf '%TY-%Tm-%Td %TH:%TM:%TS %p\n' | sort
```

## Summarize One Trial

```bash
python3 tools/compare_population_telemetry.py \
  --candidate Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_YYYYMMDD_HHMMSS.csv
```

The summary groups samples by active `SpawnerMaxEnemiesAlive`, which means one ramping run can report separate rows for caps such as `75`, `90`, `110`, `130`, and `150`.

For a fixed-cap candidate, the summary should contain one cap row. If it contains multiple cap rows, the spawn-pressure ramp was probably still enabled.

## Audit BH-009 Matrix

Use the matrix audit to see which formal fixed-cap rows are still missing:

```bash
python3 tools/audit_population_matrix.py
```

The audit only accepts telemetry files with one active cap row and enough live enemies to represent that fixed-cap workload. Ramping runs are ignored for the formal matrix.

To print the next required PIE setup:

```bash
python3 tools/audit_population_matrix.py --next
```

To write machine-readable audit output:

```bash
python3 tools/audit_population_matrix.py --json /tmp/bh009_matrix.json
```

## Compare Against A Baseline

```bash
python3 tools/compare_population_telemetry.py \
  --baseline Saved/Profiling/BH009_baseline.csv \
  --candidate Saved/Profiling/BH009_candidate.csv
```

The current named baselines are tracked in `tools/telemetry_baselines.json`. For ordinary task closeout against the accepted cap-`100` trial, use:

```bash
python3 tools/compare_population_telemetry.py \
  --baseline-profile cap-100-fixed \
  --latest
```

Default failure thresholds:

| Check | Default |
| --- | --- |
| p95 average frame time | Candidate must stay within `15%` or `2.0 ms`, whichever allows more variance |
| Peak physical memory | Candidate must stay within `256 MB` of baseline |
| Comparable workload | Candidate must reach at least `90%` of the baseline peak live-enemy count for each compared cap |

Default warning threshold:

| Check | Default |
| --- | --- |
| Max average frame time | Warns when candidate exceeds the same `15%` or `2.0 ms` tolerance |

Tune thresholds only when the reason is documented in the task closeout:

```bash
python3 tools/compare_population_telemetry.py \
  --baseline Saved/Profiling/BH009_baseline.csv \
  --candidate Saved/Profiling/BH009_candidate.csv \
  --frame-regression-ratio 1.20 \
  --frame-regression-ms 3.0 \
  --memory-regression-mb 384
```

## Task Closeout Rule

For each completed task that could affect performance:

- Run at least one candidate PIE telemetry trial.
- Compare against the current accepted baseline CSV.
- Paste the comparison result or CSV summary into the task closeout notes.
- If the comparison fails, do not mark the task complete until the regression is fixed or explicitly accepted with a rationale.

## Updating The Baseline

Promote a new CSV to baseline only after the task is accepted and the candidate comparison passes. Update `tools/telemetry_baselines.json` to point the named profile at the accepted CSV.

For local ad hoc copies:

```bash
cp Saved/Profiling/BH009_candidate.csv Saved/Profiling/BH009_baseline.csv
```

Keep long-term baseline CSVs outside source control unless a small representative sample is intentionally added for documentation. `Saved/Profiling` files are runtime artifacts.
