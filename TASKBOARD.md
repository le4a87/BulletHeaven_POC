# Bullet Heaven POC Task Board

## Purpose

This task board converts the planning-phase review into finite implementation tasks for a playable survivor-style proof of concept. It now tracks remaining and active work; completed task summaries are preserved in `docs/IMPLEMENTATION_HISTORY.md`.

## Current Gameplay Snapshot

The following observations were confirmed from the project's custom Blueprints as of the current playable prototype:

| Asset | Current State |
| --- | --- |
| `BP_Player` | Starts a looping fire timer, asks the native targeting registry for the nearest live enemy, and spawns `BP_Projectile`. It now processes enemy contact through a `0.75`-second invulnerability window, tracks run-end state, and pauses gameplay at zero health. Variables include `Health = 100`, `FireRate = 0.5`, and `ProjectileClass = BP_Projectile`. |
| `BP_Projectile` | Uses straight planar projectile motion, a visible collision-driven representation, configurable damage, finite lifespan, and applies damage to `BP_Enemy` on hit. |
| `BP_Enemy` | Character Blueprint with pursuit, health/damage/death processing, Pawn-overlap contact collision, world-space health presentation, and transient billboarding damage text. |
| `BP_EnemySpawner` | Uses a looping spawn timer and exposes `SpawnRate`, `MaxEnemiesAlive`, and `SpawnDistanceFromPlayer`; the live-enemy cap has been manually raised to `75` and default spawn distance is now `2400` for current population testing. |
| Playable pawn setup | `BP_TopDownGameMode` spawns `BP_Player` and uses `BHGameplayHUD` for runtime health, time, kill, live-enemy, and game-over readouts. `BP_Player` derives directly from `Character`, not the project C++ character class. |

The implemented gameplay assets were compiled and play-tested during prototype development. Remaining tasks below track readability, ownership, test-gate, and scalability work.

## Status Key

| Status | Meaning |
| --- | --- |
| `Ready` | Scope is defined and implementation can begin. |
| `Blocked` | Requires completion of identified dependency. |
| `In Progress` | Implementation or validation is underway. |
| `Done` | Acceptance criteria have been verified in editor/play testing, then the summary is moved to `docs/IMPLEMENTATION_HISTORY.md`. |

## Milestones

| Milestone | Outcome | Included Tasks |
| --- | --- | --- |
| M1: Combat Contact | The player automatically fires visible projectiles that hit and kill one placed enemy. | `BH-001`, `BH-002` |
| M2: Survival Loop | Enemies spawn repeatedly, chase the player, and can end a run. | `BH-003`, `BH-004` |
| M3: Readable POC | Health, elapsed survival time, and basic run metrics are visible. | `BH-005` |
| M4: Horde Readiness | Target acquisition and content ownership are suitable for increasing enemy count. | `BH-006`, `BH-007`, `BH-008` |
| M5: Population Scaling | Enemy presentation and simulation costs are reduced and a supported path to hundreds of enemies is defined. | `BH-009`, `BH-010`, `BH-011`, `BH-012`, `BH-013` |
| M6: Arena Obstacles | The combat arena includes simple randomized blockers that affect player/enemy movement, targeting, and projectile travel. | `BH-019`, `BH-020` |

---

## Completed Work

Completed task summaries for `BH-001` through `BH-008`, `BH-010`, `BH-011`, `BH-012`, `BH-014`, `BH-015`, `BH-016`, `BH-017`, `BH-018`, `BH-019`, `BH-020`, plus completed feedback fixes, have been moved to `docs/IMPLEMENTATION_HISTORY.md`.

---

## BH-009: Profile Live Enemy Population Limits

**Status:** `In Progress` - formal fixed-cap `75` and `100` baselines captured; lower fixed-cap tiers pending
**Priority:** High
**Milestone:** M5: Population Scaling
**Assets:** `BP_EnemySpawner`, test map, `BHPopulationTelemetrySubsystem.*`, `tools/compare_population_telemetry.py`, `docs/BH-009_POPULATION_PROFILE.md`, `docs/PERFORMANCE_VALIDATION.md`

### Goal

Measure the current practical live-enemy limit before optimization and identify which systems consume frame time as enemy density increases.

### Scope

- Run combat-heavy test sessions at live-enemy caps of `25`, `50`, `75`, and `100`.
- Capture `stat unit`, `stat game`, `stat blueprint`, active enemy/projectile counts, and an Unreal Insights trace for the first clearly degraded tier.
- Hold combat configuration constant while changing only the live-enemy cap.
- Record the first cap that no longer maintains the selected frame-time target.

### Acceptance Criteria

- Baseline results identify a measured supported cap rather than an assumed value.
- Results distinguish game-thread, Blueprint, rendering, collision, and projectile pressure where visible in the capture.
- The highest acceptable cap and first unacceptable cap are documented with the test hardware and configuration.

### Validation

- Complete the four population-tier runs in a fresh play session using the same combat scenario.
- Add results to this task or a linked performance note.

### Running Notes

- 2026-05-29: Manual PIE check with `MaxEnemiesAlive = 50` was reported as stable by the tester. This is an informal observation, not yet a full `BH-009` profiling pass with captured stats.
- 2026-05-29: `BP_EnemySpawner.SpawnDistanceFromPlayer` was increased from `1000` to `2400` because the earlier radius could spawn enemies inside the current `1650` spring-arm camera view.
- 2026-06-01: Tester raised `BP_EnemySpawner.MaxEnemiesAlive` from `50` to `75` after validating damaged-only enemy health bars in PIE. Treat this as the current population-test cap, not a completed formal profiling tier.
- 2026-06-01: Added `docs/BH-009_POPULATION_PROFILE.md` with the formal `25`/`50`/`75`/`100` tier matrix, required stat captures, Unreal Insights trace slot, and supported-cap decision fields. Actual performance data still needs to be collected in PIE.
- 2026-06-01: Added native `BHPopulationTelemetrySubsystem`, active only on `Map_BulletHeavenPOC`, to write one CSV per PIE run under `Saved/Profiling/BH009_*.csv`. Samples include elapsed time, average/worst frame time, average FPS, live/defeated enemies, projectile count, obstacle count, active spawner cap/rate, and process physical/virtual memory usage. CPU utilization is intentionally left to frame-time stats and Unreal Insights because those are more actionable inside Unreal.
- 2026-06-01: Added `tools/compare_population_telemetry.py` and `docs/PERFORMANCE_VALIDATION.md` as the ongoing regression suite. The script summarizes telemetry by active spawner cap and compares candidate runs against a baseline with default p95 frame-time, memory, and workload thresholds.
- 2026-06-01: Reviewed initial ramping telemetry from `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_134238.csv`. The run reached active cap `150` with p95 average frame time `28.85 ms`, under the `33.3 ms` sustained target. This is objective evidence that current ramped play remains acceptable through `150`, but it is not a substitute for the formal fixed `25`/`50`/`75`/`100` matrix.
- 2026-06-01: Added console variables `bh.SpawnPressure.Enabled` and `bh.PopulationProfile.FixedCap`. For formal fixed-cap trials, set `bh.SpawnPressure.Enabled 0` and `bh.PopulationProfile.FixedCap <cap>` before PIE; restore `bh.SpawnPressure.Enabled 1` and `bh.PopulationProfile.FixedCap 0` for normal ramping play-tests.
- 2026-06-01: Reviewed formal fixed-cap `75` telemetry from `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_145802.csv`. The run held cap `75` for `203` samples from `3.0-206.6s`, reached peak live enemies `75`, averaged `67.4 FPS`, recorded p95 average frame time `16.71 ms`, max one-second average frame time `17.35 ms`, worst frame `43.42 ms`, peak projectiles `5`, `18` active obstacles, and peak physical memory `4728.9 MB`. This passes the sustained `33.3 ms` target and is usable as the cap-`75` fixed baseline.
- 2026-06-01: Reviewed formal fixed-cap `100` telemetry from `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_150403.csv`. The run held cap `100` for `236` samples from `3.0-239.9s`, reached peak live enemies `100`, averaged `63.9 FPS`, recorded p95 average frame time `20.65 ms`, max one-second average frame time `21.92 ms`, worst frame `53.64 ms`, peak projectiles `5`, `18` active obstacles, and peak physical memory `4892.6 MB`. This passes the sustained `33.3 ms` target and is usable as the cap-`100` fixed baseline.
- 2026-06-01: Added `tools/telemetry_baselines.json` and `--baseline-profile` / `--latest` options to `tools/compare_population_telemetry.py`, so future task closeout can compare the newest telemetry capture against the accepted cap-`100` baseline with `python3 tools/compare_population_telemetry.py --baseline-profile cap-100-fixed --latest`.
- 2026-06-01: Added `tools/audit_population_matrix.py` to scan `Saved/Profiling` and report which formal fixed-cap `25`/`50`/`75`/`100` rows are present. The audit accepts only single-cap telemetry files that reach the configured workload threshold, so ramping captures are not mistaken for formal fixed-cap rows.
- 2026-06-01: Extended `tools/audit_population_matrix.py` with `--next` to print the next missing fixed-cap PIE console setup and `--json` for machine-readable audit output.

---

## BH-013: Define Architecture For Hundreds Of Enemies

**Status:** `Blocked` by `BH-009`
**Priority:** Medium
**Milestone:** M5: Population Scaling
**Assets:** Architecture note and prototype assets/code as selected

### Goal

Select and validate an architecture suitable for several hundred simultaneous basic enemies rather than incrementally extending full Blueprint `Character` actors.

### Scope

- Evaluate a centralized native simulation or Unreal Mass Entity approach for movement, targeting, separation, damage application, and visibility.
- Define pooling strategy for enemies, projectiles, and transient presentation effects.
- Define cheaper rendering/collision policies, including simplified visuals, spatial queries, and removal of per-entity widgets.
- Build the smallest prototype necessary to validate the selected direction against a documented hundreds-of-enemies target.

### Acceptance Criteria

- A target population, frame-time budget, and target hardware are explicit.
- The selected architecture is justified using the earlier profiling evidence.
- A prototype reaches the selected high-population scenario or produces quantified evidence for the next design revision.

### Validation

- Run the prototype with the target population and record simulation, rendering, collision, and feedback behavior.

---

## Recommended Execution Order

| Order | Task | Reason |
| --- | --- | --- |
| 1 | `BH-009` | Establishes the measured population ceiling and identifies the first actual bottleneck after basic tuning. |
| 2 | `BH-012` | Addresses persistent per-enemy Tick, movement, and animation costs based on measured evidence. |
| 3 | `BH-013` | Selects the architecture required only if the target is several hundred enemies. |

## Deferred Backlog

These features are intentionally excluded from the initial proof-of-concept tasks and should not be started until the POC play-test gate provides evidence that the core loop is sound:

- Experience drops, leveling, and upgrade selection.
- Multiple weapons or projectile behaviors.
- Enemy archetypes, bosses, and authored wave schedules.
- Object pooling and deeper optimization.
- High-population enemy architecture implementation beyond the validated `BH-013` prototype.
- Audio, VFX, animation, and presentation polish.
- Persistence, meta progression, and packaged-build concerns.
