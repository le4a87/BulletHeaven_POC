# Bullet Heaven POC Task Board

## Purpose

This task board converts the planning-phase review into finite implementation tasks for a playable survivor-style proof of concept. It now tracks remaining and active work; completed task summaries are preserved in `docs/IMPLEMENTATION_HISTORY.md`.

## Current Gameplay Snapshot

The following observations were confirmed from the project's custom Blueprints as of the current playable prototype:

| Asset | Current State |
| --- | --- |
| `BP_Player` | Starts a looping fire timer, scans all `BP_Enemy` actors, selects the nearest target, and spawns `BP_Projectile`. It now processes enemy contact through a `0.75`-second invulnerability window, tracks run-end state, and pauses gameplay at zero health. Variables include `Health = 100`, `FireRate = 0.5`, and `ProjectileClass = BP_Projectile`. |
| `BP_Projectile` | Uses straight planar projectile motion, a visible collision-driven representation, configurable damage, finite lifespan, and applies damage to `BP_Enemy` on hit. |
| `BP_Enemy` | Character Blueprint with pursuit, health/damage/death processing, Pawn-overlap contact collision, world-space health presentation, and transient billboarding damage text. |
| `BP_EnemySpawner` | Uses a looping spawn timer and exposes `SpawnRate`, `MaxEnemiesAlive`, and `SpawnDistanceFromPlayer`; the current live-enemy cap defaults to `10`. |
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

Completed task summaries for `BH-001` through `BH-005`, plus completed feedback fixes, have been moved to `docs/IMPLEMENTATION_HISTORY.md`.

---

## BH-006: Replace Repeated Global Target Scans

**Status:** `Ready`
**Priority:** Medium  
**Milestone:** M4: Horde Readiness  
**Assets:** `BP_Player`, `BP_EnemySpawner` or selected registry owner

### Goal

Keep automatic aiming viable as enemy count and firing frequency increase by avoiding a world-wide actor class query on every shot.

### Research Required

- Measure current `Get All Actors Of Class` approach with representative loads such as 25, 100, and 250 live enemies and faster firing intervals.
- Evaluate registry iteration against spatial overlap queries within a targeting radius; select based on the expected weapon targeting rules.
- Decide how weapons retrieve candidates without tightly coupling every future weapon to the spawner implementation.
- Define behavior when the current target dies between target selection and firing.

### Scope

- Remove `Get All Actors Of Class (BP_Enemy)` from the repeated player firing path.
- Read candidates from the selected active-enemy source or bounded query.
- Keep nearest-valid-target behavior functionally equivalent for the base weapon.
- Handle invalid/destroyed candidates without runtime errors.

### Out Of Scope

- Sophisticated spatial partitioning, multithreaded systems, mass entity conversion, target priorities, chain attacks, or full weapon framework.

### Acceptance Criteria

- Base weapon still fires toward the nearest valid enemy.
- Firing works as enemies are spawned and destroyed continuously.
- The firing path no longer performs a global all-enemy actor lookup per shot.
- A documented play-test load does not show obvious stalling from target selection.

### Validation

- Run tests with increasing active enemy caps and faster `FireRate`.
- Record rough editor frame behavior and the chosen practical cap for the POC.

---

## BH-007: Decide And Apply Player Blueprint Ownership Strategy

**Status:** `Ready` for decision; implementation should occur before significant player features expand  
**Priority:** Medium  
**Milestone:** M4: Horde Readiness  
**Assets:** `BP_Player`, `Source/BulletHeavenPOC/BulletHeavenPOCCharacter.*`, `BP_TopDownGameMode`

### Goal

Remove uncertainty about whether the playable character is authored in Blueprint alone or extends the existing native character base.

### Research Required

- Compare the components and movement/camera setup of `BP_Player` with `ABulletHeavenPOCCharacter`.
- Identify which future systems are expected to live in C++ versus Blueprint for this POC.
- Evaluate reparenting risk: inherited components, defaults, input behavior, and existing player graph compatibility.
- Confirm the GameMode and PlayerController combination required for the chosen movement scheme.

### Scope

- Make an explicit choice: keep `BP_Player` Blueprint-only, or reparent it to `ABulletHeavenPOCCharacter`.
- If reparenting is selected, verify component duplication/conflicts, retained targeting behavior, camera behavior, and movement input.
- Document the selected ownership rule for further gameplay additions.

### Out Of Scope

- General conversion of Blueprint logic to C++, gameplay framework redesign, multiplayer architecture, or template-content cleanup.

### Acceptance Criteria

- The player inheritance strategy is documented in this file or a project architecture note.
- Playable pawn spawns correctly through the configured GameMode.
- Movement, camera, and automatic projectile firing still operate after any required change.

### Validation

- Start a run from the configured default map and exercise movement and firing.
- Recompile affected Blueprint and native project targets if C++ ownership is selected.

---

## BH-008: Establish POC Play-Test And Performance Gate

**Status:** `Blocked` by `BH-006`
**Priority:** Medium  
**Milestone:** M4: Horde Readiness  
**Assets:** Documentation and any debug configuration required for testing

### Goal

Define a finite acceptance test for declaring the proof of concept successful before expanding content.

### Research Required

- Choose a minimum target run duration and enemy density that represent the desired survivor feel.
- Use Unreal profiling/debug tooling to identify the key budget categories: Blueprint game time, active actor count, collision, and projectile count.
- Determine a practical frame-rate or frame-time target for the intended development machine.
- Identify the maximum projectile/enemy counts reached during the test and whether cleanup behavior is stable.

### Scope

- Write a short repeatable play-test checklist.
- Capture target parameters: fire rate, spawn rate, live enemy cap, enemy health, player health, and survival duration.
- Record whether the chosen test passes functional and performance expectations.
- File follow-up tasks only for failures observed in the defined test.

### Out Of Scope

- Shipping optimization, platform certification, automated performance infrastructure, packaging, or content balancing beyond the test scenario.

### Acceptance Criteria

- A tester can start from `Map_BulletHeavenPOC` and execute the defined run without editor-only setup.
- The run includes automatic firing, enemy kills, repeated spawning, player damage, and a game-over result.
- No unbounded projectile/enemy accumulation or recurring Blueprint runtime errors occur.
- Test results record approximate peak counts and observed frame behavior.

### Validation

- Complete the documented run once from a clean editor play session and record results below.

### Test Results

| Date | Build/Configuration | Result | Notes |
| --- | --- | --- | --- |
| Pending | Pending | Pending | Establish after `BH-006` is complete. |

---

## BH-009: Profile Live Enemy Population Limits

**Status:** `Ready`
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

---

## BH-010: Gate Damage-Text Billboard And Fade Updates

**Status:** `Ready`
**Priority:** High
**Milestone:** M5: Population Scaling
**Assets:** `Content/Blueprints/Enemies/BP_Enemy.uasset`, damage-text presentation owner if refactored

### Goal

Remove hidden floating-damage text from steady-state per-enemy Tick cost.

### Scope

- Update the camera-facing rotation only while damage text is visible, or move transient text display to a pooled presentation manager.
- Preserve the current short-lived translucent fade behavior and camera-facing readability.
- Ensure repeated hits refresh the display duration without accumulating latent updates or text actors.

### Acceptance Criteria

- Enemies that are not currently showing damage text perform no billboard work for that text.
- Damage text still faces the camera, fades quickly, and hides reliably after repeated hits.
- Performance comparison at the `BH-009` degraded tier shows the change does not add frame cost and reduces damage-feedback overhead under sustained hits.

### Validation

- Test isolated hits, rapid repeated hits, and a dense combat case.
- Compare Blueprint/game-thread observations before and after the change.

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

**Status:** `Blocked` by `BH-009`, `BH-010`, and `BH-011`
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

## Recommended Execution Order

| Order | Task | Reason |
| --- | --- | --- |
| 1 | `BH-006` | Prevents the current targeting strategy from defining the horde ceiling. |
| 2 | `BH-007` | Establishes code ownership before player behavior expands substantially. This decision may be executed earlier if C++ gameplay work begins. |
| 3 | `BH-008` | Defines the POC completion gate after the loop exists. |
| 4 | `BH-009` | Establishes the measured population ceiling and identifies the first actual bottleneck. |
| 5 | `BH-010` | Removes unnecessary hidden damage-text updates from every live enemy. |
| 6 | `BH-011` | Reduces a likely high-cost per-enemy world-widget burden. |
| 7 | `BH-012` | Addresses persistent per-enemy Tick, movement, and animation costs based on measured evidence. |
| 8 | `BH-013` | Selects the architecture required only if the target is several hundred enemies. |

## Deferred Backlog

These features are intentionally excluded from the initial proof-of-concept tasks and should not be started until `BH-008` provides evidence that the core loop is sound:

- Experience drops, leveling, and upgrade selection.
- Multiple weapons or projectile behaviors.
- Enemy archetypes, bosses, and authored wave schedules.
- Object pooling and deeper optimization.
- High-population enemy architecture implementation beyond the validated `BH-013` prototype.
- Audio, VFX, animation, and presentation polish.
- Persistence, meta progression, and packaged-build concerns.
