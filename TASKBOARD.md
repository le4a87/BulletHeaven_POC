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
| `BP_EnemySpawner` | Uses a looping spawn timer and exposes `SpawnRate`, `MaxEnemiesAlive`, and `SpawnDistanceFromPlayer`; the live-enemy cap has been manually raised to `50` and default spawn distance is now `2400` for current population testing. |
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

---

## Completed Work

Completed task summaries for `BH-001` through `BH-008`, `BH-010`, `BH-014`, `BH-015`, plus completed feedback fixes, have been moved to `docs/IMPLEMENTATION_HISTORY.md`.

---

## BH-009: Profile Live Enemy Population Limits

**Status:** `In Progress` - informal 50-enemy PIE observation recorded; formal profiling pending
**Priority:** High
**Milestone:** M5: Population Scaling
**Assets:** `BP_EnemySpawner`, test map, performance test notes

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

---

## BH-011: Reduce Per-Enemy Health-Bar Widget Cost

**Status:** `Ready`
**Priority:** High
**Milestone:** M5: Population Scaling
**Assets:** `Content/Blueprints/Enemies/BP_Enemy.uasset`, `Content/Blueprints/UI/WBP_ActorHealthBar.uasset`

### Goal

Avoid paying for a world-space UMG health bar on every live enemy throughout combat.

### Scope

- Choose a visibility policy such as damaged-only, nearby-only, selected/targeted-only, or a bounded combination.
- Disable, remove, or pool health-bar widget work for enemies outside that policy.
- Retain readable health feedback for enemies the player is actively engaging.

### Acceptance Criteria

- Large groups do not maintain visible/ticking health-bar widgets for every enemy.
- Health bars appear and update correctly under the selected visibility policy.
- Re-run of the `BH-009` scenario records the population-tier improvement or residual widget cost.

### Validation

- Test damage, recovery/hide timing if applicable, target changes, and crowded enemy groups.

---

## BH-012: Reduce Per-Enemy Tick And Character Cost

**Status:** `Blocked` by `BH-009` and `BH-011`
**Priority:** Medium
**Milestone:** M5: Population Scaling
**Assets:** `BP_Enemy`, enemy movement/animation assets, optional native gameplay code

### Goal

Determine the least disruptive simulation changes needed after presentation costs have been removed from the measured bottleneck.

### Scope

- Review the cost of each enemy being a full `Character` with `CharacterMovement`, skeletal mesh, collision, and zero-interval Blueprint Tick.
- Apply tick-rate or distance-based throttling where behavior remains acceptable.
- Configure off-screen animation and visual update policies.
- If still required by profiling, prototype a lightweight basic-enemy `Pawn` or `Actor` movement implementation instead of `CharacterMovement`.

### Acceptance Criteria

- The chosen enemy representation and tick policy are documented with measured before/after results.
- Nearby combat remains responsive while distant or off-screen enemies incur reduced update cost.
- No collision, pursuit, damage, or death regressions appear in the representative load test.

### Validation

- Re-run population tests and compare frame-time categories against `BH-009`.
- Play-test close-range swarms and off-screen approach behavior.

---

## BH-013: Define Architecture For Hundreds Of Enemies

**Status:** `Blocked` by `BH-009` through `BH-012`
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

## BH-016: Improve HUD Timer And Kill Stat Visibility

**Status:** `In Progress` - native HUD implementation complete; PIE readability check pending
**Priority:** High
**Milestone:** M3: Readable POC
**Assets:** `BHGameplayHUD.*`

### Goal

Make elapsed time and kill statistics easy to read during combat without distracting from gameplay.

### Scope

- Increase visual prominence of timer and kill count through position, font size, contrast, spacing, or grouping.
- Preserve health, live-enemy count, and game-over messaging.
- Keep HUD layout readable across the editor play viewport sizes used for testing.

### Acceptance Criteria

- Timer and kill count are immediately visible during active combat.
- HUD text has sufficient contrast over the map floor, enemies, and projectile effects.
- No HUD elements overlap or clip in common PIE viewport sizes.

### Validation

- Rebuild the native target if HUD C++ changes are made.
- Play-test at normal and crowded enemy counts and verify readability while moving.

---

## BH-017: Scale Enemy Spawn Pressure Over Time

**Status:** `Ready`
**Priority:** High
**Milestone:** M5: Population Scaling
**Assets:** `BP_EnemySpawner`, `BHGameplayHUD.*` or elapsed-time provider if needed

### Goal

Increase run pressure by raising `MaxEnemiesAlive` and spawn frequency as elapsed survival time increases.

### Scope

- Define a simple time-based spawn curve or tier table for live-enemy cap and spawn interval.
- Increase `MaxEnemiesAlive` over time instead of relying on one static cap.
- Increase spawn frequency over time while preserving a minimum interval that avoids runaway spawning.
- Keep current defaults suitable for short editor smoke tests.

### Acceptance Criteria

- Enemy pressure clearly ramps during a multi-minute run.
- Spawn settings remain bounded and inspectable on `BP_EnemySpawner`.
- The spawner does not exceed the active cap or create unbounded actor accumulation.
- The ramp design records target caps, intervals, and elapsed-time breakpoints.

### Validation

- Compile/save `BP_EnemySpawner`.
- Play-test a run long enough to cross at least two spawn-pressure tiers.
- Record observed peak live-enemy count and any frame-time concerns for `BH-009`.

---

## BH-018: Prevent Enemy Stacking And Excessive Overlap

**Status:** `Ready`
**Priority:** High
**Milestone:** M5: Population Scaling
**Assets:** `BP_Enemy`, enemy movement/collision settings, optional native movement helper

### Goal

Keep enemies from occupying the same space so swarms read as a crowd instead of a single stacked pile.

### Scope

- Review current enemy capsule collision, movement collision response, and spawn placement behavior.
- Choose a separation approach appropriate for the current `Character`-based enemy: collision tuning, simple avoidance, spawn spacing, or a bounded combination.
- Preserve enemies' ability to surround and pressure the player without hard-blocking all movement.
- Avoid expensive per-enemy neighbor scans that would undermine population scaling.

### Acceptance Criteria

- Multiple enemies chasing the player do not visibly stack on top of each other during normal pursuit.
- Enemies can still move around each other well enough to maintain pressure.
- Spawned enemies do not begin play in severe overlap unless immediately resolved.
- The approach remains stable at the current and next planned live-enemy caps.

### Validation

- Compile/save `BP_Enemy` and any modified movement or collision assets.
- Play-test dense chase scenarios at `25+` live enemies.
- Re-check performance notes for `BH-009` if the solution adds per-enemy avoidance work.

---

## Recommended Execution Order

| Order | Task | Reason |
| --- | --- | --- |
| 1 | `BH-016` | Makes key run metrics readable during normal and crowded play. |
| 2 | `BH-017` | Adds time-based pressure so population testing reflects a survivor-style run. |
| 3 | `BH-018` | Prevents crowd readability and collision behavior from distorting population tests. |
| 4 | `BH-009` | Establishes the measured population ceiling and identifies the first actual bottleneck after basic tuning. |
| 5 | `BH-011` | Reduces a likely high-cost per-enemy world-widget burden. |
| 6 | `BH-012` | Addresses persistent per-enemy Tick, movement, and animation costs based on measured evidence. |
| 7 | `BH-013` | Selects the architecture required only if the target is several hundred enemies. |

## Deferred Backlog

These features are intentionally excluded from the initial proof-of-concept tasks and should not be started until the POC play-test gate provides evidence that the core loop is sound:

- Experience drops, leveling, and upgrade selection.
- Multiple weapons or projectile behaviors.
- Enemy archetypes, bosses, and authored wave schedules.
- Object pooling and deeper optimization.
- High-population enemy architecture implementation beyond the validated `BH-013` prototype.
- Audio, VFX, animation, and presentation polish.
- Persistence, meta progression, and packaged-build concerns.
