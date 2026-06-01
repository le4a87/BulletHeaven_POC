# BH-009 Population Profile

Use this worksheet for the formal live-enemy population pass. The goal is to identify the highest supported cap and first clearly degraded cap using the same combat scenario for every tier.

## Test Configuration

| Field | Value |
| --- | --- |
| Date | TBD |
| Tester | TBD |
| Hardware | TBD |
| Build | PIE, Development Editor |
| Map | `Content/Maps/Map_BulletHeavenPOC` |
| Enemy cap tiers | `25`, `50`, `75`, `100` |
| Frame-time target | Keep sustained game-thread time at or below `33.3 ms` |
| Current obstacle field | Runtime `BHObstacleFieldSubsystem` enabled |
| Current spawn pressure | Disable for fixed-cap trials with `bh.SpawnPressure.Enabled 0`; enable only for ramp trials |
| Fixed-cap override | Set with `bh.PopulationProfile.FixedCap <cap>` before PIE |
| CSV telemetry | `BHPopulationTelemetrySubsystem` writes to `Saved/Profiling/BH009_*.csv` |
| Regression suite | `tools/compare_population_telemetry.py`; matrix audit in `tools/audit_population_matrix.py`; named baselines in `tools/telemetry_baselines.json`; workflow documented in `docs/PERFORMANCE_VALIDATION.md` |

## Setup

1. Build `BulletHeavenPOCEditor` if native source changed since the editor was opened.
2. Restart Unreal Editor after native builds so PIE uses the current module.
3. Open `Content/Maps/Map_BulletHeavenPOC`.
4. Before each fixed-cap trial, set console variable `bh.SpawnPressure.Enabled 0` so `BHSpawnPressureSubsystem` does not ramp the cap during the run.
5. For each test row, set console variable `bh.PopulationProfile.FixedCap <cap>` before PIE. Example: `bh.PopulationProfile.FixedCap 100`.
6. Use a fresh PIE session for each cap.
7. Hold all other combat settings constant.
8. Enable these console stats during the active combat window:
   - `stat fps`
   - `stat unit`
   - `stat game`
   - `stat blueprint`
9. For the first tier that is clearly unacceptable, capture an Unreal Insights trace and record the trace file path.
10. Restore `bh.SpawnPressure.Enabled 1` and `bh.PopulationProfile.FixedCap 0` after fixed-cap profiling if you want normal survivor pressure for general play-testing.

## Capture Rules

- Run each tier for `3` minutes after enemies begin spawning, or until game over.
- Record typical values during active combat, not editor startup hitches.
- Treat one-frame hitches separately from sustained frame-time problems.
- Record live enemy count from the HUD.
- Estimate simultaneous projectiles visually or through runtime actor inspection if practical.
- Do not change the obstacle layout policy, spawn distance, player damage, projectile damage, or enemy health between tiers.
- Save the generated `Saved/Profiling/BH009_*.csv` path for each tier. The CSV samples elapsed time, average/worst frame time, average FPS, live/defeated enemy counts, projectile count, obstacle count, active spawner cap/rate, and process memory usage.
- Compare the candidate CSV against the accepted baseline using `tools/compare_population_telemetry.py` after completing any task that could affect runtime performance.
- Run `python3 tools/audit_population_matrix.py` to confirm which fixed-cap `25`/`50`/`75`/`100` rows are present.
- Run `python3 tools/audit_population_matrix.py --next` to print the next missing fixed-cap setup.

## Results

| Cap | Duration | Result | Telemetry CSV | Peak Live Enemies | Peak Projectiles | Typical FPS | Worst Sustained FPS | Typical Game Thread | Worst Sustained Game Thread | Peak Memory | Blueprint / Rendering / Collision / Projectile Notes | Issues |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `25` | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| `50` | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD | TBD |
| `75` | `3.4 min` | Pass | `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_145802.csv` | `75` | `5` | `67.4` | `57.6` | `14.92 ms` | `17.35 ms` | `4728.9 MB` | Fixed-cap run with `bh.SpawnPressure.Enabled 0` and `bh.PopulationProfile.FixedCap 75`; `18` obstacles active; p95 average frame time `16.71 ms` | None |
| `100` | `4.0 min` | Pass | `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_150403.csv` | `100` | `5` | `63.9` | `45.6` | `15.99 ms` | `21.92 ms` | `4892.6 MB` | Fixed-cap run with `bh.SpawnPressure.Enabled 0` and `bh.PopulationProfile.FixedCap 100`; `18` obstacles active; p95 average frame time `20.65 ms` | None |

## Accepted Regression Baselines

| Profile | Telemetry CSV | Purpose |
| --- | --- | --- |
| `cap-75-fixed` | `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_145802.csv` | Formal fixed-cap `75` reference. |
| `cap-100-fixed` | `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_152150.csv` | Current cap-`100` regression baseline after `BH-012` passed telemetry comparison. |
| `cap-100-fixed-pre-bh012` | `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_150403.csv` | Historical cap-`100` before/after reference for `BH-012`. |

## Supported Cap Decision

| Field | Value |
| --- | --- |
| Highest acceptable cap | Formal fixed-cap baseline currently passes at `100`; initial ramp telemetry reached `150` without sustained frame-time failure |
| First unacceptable cap | TBD |
| Primary bottleneck | TBD; fixed `100` cap leaves sustained frame-time headroom, while initial ramp telemetry shows frame time rising with live enemy cap but still below `33.3 ms` p95 at `150` |
| Unreal Insights trace for first unacceptable cap | TBD |
| Follow-up task needed | TBD |

## Initial Ramping Telemetry

This run used the current spawn-pressure ramp rather than fixed `25`/`50`/`75`/`100` cap sessions, so it is objective evidence but not a replacement for the formal tier matrix above.

| Field | Value |
| --- | --- |
| Date | 2026-06-01 |
| Telemetry CSV | `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_134238.csv` |
| Summary command | `python3 tools/compare_population_telemetry.py --candidate Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_134238.csv` |
| Result | No sustained degradation observed through the `150` spawn-pressure tier |

| Active Cap | Samples | Window | Avg FPS | p95 Avg Frame | Max Avg Frame | Worst Frame | Peak Live | Peak Projectiles | Peak Physical MB |
| ---: | ---: | --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `75` | `57` | `3.0-59.4s` | `72.1` | `14.58 ms` | `14.90 ms` | `38.51 ms` | `34` | `5` | `4856.0` |
| `90` | `60` | `60.4-119.9s` | `62.2` | `17.89 ms` | `18.04 ms` | `35.55 ms` | `90` | `1` | `4918.1` |
| `110` | `59` | `120.9-179.5s` | `50.9` | `21.23 ms` | `21.30 ms` | `49.41 ms` | `110` | `1` | `4931.2` |
| `130` | `59` | `180.5-239.2s` | `42.4` | `25.27 ms` | `25.51 ms` | `46.22 ms` | `130` | `1` | `4957.2` |
| `150` | `29` | `240.2-268.6s` | `37.1` | `28.85 ms` | `28.92 ms` | `59.02 ms` | `150` | `1` | `4985.6` |

### Initial Interpretation

- The `150` tier stayed under the `33.3 ms` sustained frame-time target by p95 average frame time.
- Worst-frame spikes above `33.3 ms` occurred, but the one-second average frame-time samples did not indicate sustained failure.
- Peak physical memory rose to roughly `4986 MB`; this should be watched in longer steady-state trials but does not by itself indicate a leak.
- Projectile pressure was low at high caps (`0-1` peak in later tiers), so this run primarily stresses enemy count, movement, separation, obstacles, targeting, and HUD/feedback rather than projectile accumulation.
- Recommended next capture is a longer steady-state run at cap `150`, then `175` or `200` if `150` remains stable.

## Notes

- The earlier `50`-enemy PIE observation was informal and should not be counted as the formal `BH-009` baseline.
- The current working cap has been raised to `75` for general play-testing, but this worksheet should still run `25`, `50`, `75`, and `100` so the result distinguishes supported and unsupported tiers.
- The spawn-pressure ramp intentionally changes cap over time; fixed-cap `BH-009` trials must disable it with `bh.SpawnPressure.Enabled 0` and set the desired cap with `bh.PopulationProfile.FixedCap <cap>` before PIE starts.
