# Bullet Heaven POC Task Board

## Purpose

This task board converts the planning-phase review into finite implementation tasks for a playable survivor-style proof of concept. Tasks are ordered to produce the smallest functional combat loop first, then expand enemy pressure, player failure states, feedback, and scalability.

## Current Gameplay Snapshot

The following observations were confirmed from the project's custom Blueprints as of the current playable prototype:

| Asset | Current State |
| --- | --- |
| `BP_Player` | Starts a looping fire timer, scans all `BP_Enemy` actors, selects the nearest target, and spawns `BP_Projectile`. It now processes enemy contact through a `0.75`-second invulnerability window, tracks run-end state, and pauses gameplay at zero health. Variables include `Health = 100`, `FireRate = 0.5`, and `ProjectileClass = BP_Projectile`. |
| `BP_Projectile` | Uses straight planar projectile motion, a visible collision-driven representation, configurable damage, finite lifespan, and applies damage to `BP_Enemy` on hit. |
| `BP_Enemy` | Character Blueprint with pursuit, health/damage/death processing, Pawn-overlap contact collision, world-space health presentation, and transient billboarding damage text. |
| `BP_EnemySpawner` | Uses a looping spawn timer and exposes `SpawnRate`, `MaxEnemiesAlive`, and `SpawnDistanceFromPlayer`; the current live-enemy cap defaults to `10`. |
| Playable pawn setup | `BP_TopDownGameMode` spawns `BP_Player`. `BP_Player` derives directly from `Character`, not the project C++ character class. |

The implemented gameplay assets were compiled and play-tested during prototype development. Remaining tasks below track missing survival-loop behavior, readability, and scalability work.

## Status Key

| Status | Meaning |
| --- | --- |
| `Ready` | Scope is defined and implementation can begin. |
| `Blocked` | Requires completion of identified dependency. |
| `In Progress` | Implementation or validation is underway. |
| `Done` | Acceptance criteria have been verified in editor/play testing. |

## Milestones

| Milestone | Outcome | Included Tasks |
| --- | --- | --- |
| M1: Combat Contact | The player automatically fires visible projectiles that hit and kill one placed enemy. | `BH-001`, `BH-002` |
| M2: Survival Loop | Enemies spawn repeatedly, chase the player, and can end a run. | `BH-003`, `BH-004` |
| M3: Readable POC | Health, elapsed survival time, and basic run metrics are visible. | `BH-005` |
| M4: Horde Readiness | Target acquisition and content ownership are suitable for increasing enemy count. | `BH-006`, `BH-007`, `BH-008` |
| M5: Population Scaling | Enemy presentation and simulation costs are reduced and a supported path to hundreds of enemies is defined. | `BH-009`, `BH-010`, `BH-011`, `BH-012`, `BH-013` |

---

## BH-001: Implement Functional Projectile

**Status:** `Done`
**Priority:** Critical  
**Milestone:** M1: Combat Contact  
**Assets:** `Content/Blueprints/Projectiles/BP_Projectile.uasset`, optionally projectile visual assets

### Goal

Make every projectile spawned by `BP_Player` visible, planar, collision-capable, damaging, and self-cleaning.

### Research Required

- Inspect the collision presets already used by the player capsule and enemy capsule, then select a projectile collision profile that overlaps enemies without colliding with the player.
- Determine whether the proof of concept should use `Apply Damage` or a direct typed call such as `ReceiveProjectileHit`; prefer the smallest contract that supports later enemy variants.
- Confirm whether the projectile graphic should use a primitive mesh, sprite/flipbook, or existing project material for the first playable loop.
- Confirm top-down plane behavior by testing `ProjectileGravityScale = 0` and spawn height relative to enemy collision.

### Scope

- Add a collision primitive as the projectile root or collision-driving component.
- Add a minimal visible representation.
- Configure `ProjectileMovement` for straight top-down travel with gravity disabled.
- Add configurable projectile `Damage` and finite `LifeSpan` values.
- On valid enemy collision, deliver damage once and destroy the projectile.
- Destroy projectiles that never hit before they accumulate outside the play area.

### Out Of Scope

- Piercing, ricochet, homing, area damage, elemental modifiers, or pooled projectiles.
- Weapon upgrade logic.
- Enemy death rewards.

### Acceptance Criteria

- A projectile is visible after it spawns.
- A projectile travels horizontally toward the targeted enemy rather than falling.
- A projectile collides with `BP_Enemy` and invokes the selected damage interface exactly once.
- A hit projectile destroys itself.
- A missed projectile destroys itself after a bounded lifetime.
- Blueprint compiles without warnings or errors.

### Validation

- Place one `BP_Enemy` in the test map and run Play In Editor.
- Observe projectiles traveling to and contacting the enemy.
- Allow shots to miss or remove the enemy, then verify old projectiles do not remain indefinitely.

### Implementation Record

- Completed on 2026-05-23.
- Added a `CollisionSphere` query-overlap root and a visible sphere mesh child to `BP_Projectile`.
- Configured straight planar projectile motion at speed `1200` with gravity disabled.
- Added editable `Damage = 10` and finite `InitialLifeSpan = 3` defaults.
- Added the hit path `OnComponentBeginOverlap (CollisionSphere)` -> `Cast To BP_Enemy` -> `Apply Damage` -> `Destroy Actor`.
- Compiled with warnings treated as errors and saved the Blueprint.
- Ran PIE with the placed enemy present; live projectiles were observed through runtime actor inspection and no Blueprint runtime errors were reported.
- Actual enemy health reduction/death is deferred to `BH-002`, where `BP_Enemy` will consume the standard damage event.

---

## BH-002: Implement Enemy Health, Pursuit, And Death

**Status:** `Done`  
**Priority:** Critical  
**Milestone:** M1: Combat Contact  
**Assets:** `Content/Blueprints/Enemies/BP_Enemy.uasset`

### Goal

Turn `BP_Enemy` from a selectable placeholder into a hostile unit that can pursue the player, receive projectile damage, and die.

### Research Required

- Decide the simplest pursuit implementation for initial hordes: direct planar movement toward player, AI Move To with navigation, or a native movement path. Compare cost and expected enemy counts before committing.
- Inspect whether `Map_BulletHeavenPOC` provides sufficient navigable play space if nav-based movement is selected.
- Define the damage contract used by `BH-001`, including health type, death event, and prevention of duplicate death processing.
- Determine whether enemy-player contact should be handled by the enemy capsule overlap or reserved for the player damage task.

### Scope

- Add configurable `MaxHealth`, current health initialization, movement speed, and contact damage values.
- Find/store the player reference at a controlled point such as `BeginPlay`, with validity handling.
- Implement movement toward the player suitable for the selected proof-of-concept approach.
- Receive projectile damage and decrement health.
- On zero health, stop participation in targeting/collision and destroy the enemy.
- Expose a death hook or event usable later by counters, experience, and wave logic.

### Out Of Scope

- Multiple enemy archetypes, attacks beyond contact damage, hit reactions, loot, experience pickups, animations, or object pooling.

### Acceptance Criteria

- A placed enemy moves toward the player during play.
- Projectile hits reduce enemy health.
- Enemy destruction occurs once when health reaches zero.
- Dead enemies no longer remain selectable as a target.
- Blueprint compiles without warnings or errors.

### Validation

- Test one enemy with health requiring multiple hits.
- Test lethal and non-lethal hits.
- Run with no valid player target and verify no repeated runtime errors occur.

### Implementation Record

- Completed on 2026-05-23.
- Selected direct `CharacterMovement` pursuit for the POC rather than navigation, avoiding NavMesh dependencies before timed spawning work.
- Added editable `MaxHealth = 30`, `MoveSpeed = 250`, and `ContactDamage = 10`, plus runtime `CurrentHealth`, `PlayerTarget`, and `IsDead` state.
- Added `OnEnemyDied` as an event dispatcher for future counters, experience, and spawn tracking.
- Initialized health and cached a valid `BP_Player` target in `BeginPlay`; valid-target guarded Tick movement uses `Add Movement Input`.
- Added `Event AnyDamage` processing, single-death guarding, death dispatch, and actor destruction on lethal projectile damage.
- Compiled with warnings treated as errors and ran PIE with the placed enemy; after the multi-hit firing window the enemy was destroyed with no Blueprint/script runtime errors logged.

---

## BH-003: Implement Timed Enemy Spawning And Enemy Registry

**Status:** `Done`
**Priority:** High  
**Milestone:** M2: Survival Loop  
**Assets:** `Content/Blueprints/Spawners/BP_EnemySpawner.uasset`, `Content/Maps/Map_BulletHeavenPOC.umap`

### Goal

Create renewable enemy pressure around the player and establish ownership of active enemy references for later targeting and metrics.

### Research Required

- Choose the spawn geometry for the POC: ring around the player, map edge points, or authored spawn points.
- Determine safe spawn distance bounds so enemies appear off-screen or outside immediate contact range while remaining on valid walkable ground.
- Decide where the active enemy registry belongs for this scope: spawner-owned array, game-state/game-mode system, or separate combat director Blueprint.
- Verify how destroyed enemies notify the registry so stale references are pruned reliably.
- Define initial difficulty knobs: interval, batch size, maximum live enemy count, and optional elapsed-time scaling.

### Scope

- Place one configured spawner in the playable map.
- Add enemy class, spawn interval, minimum/maximum spawn radius, and maximum active enemy count settings.
- Spawn enemies repeatedly at valid positions around the player.
- Maintain an array or equivalent collection of active enemies and remove invalid/dead entries.
- Add minimal difficulty ramping only if it is a simple exposed value change over elapsed time.

### Out Of Scope

- Authored waves, bosses, biome zones, director AI, elite modifiers, object pooling, or save/load state.

### Acceptance Criteria

- Starting the level produces enemies without manual placement.
- New enemies originate outside an immediate unsafe radius around the player.
- Live enemy count respects the configured cap.
- Destroyed enemies are removed from the active collection.
- Extended play does not produce stale-reference runtime errors.

### Validation

- Test start-of-run spawn behavior and cap behavior.
- Kill enemies repeatedly and verify replacements appear.
- Run for at least two minutes with debug live-enemy count visible or logged.

### Implementation Record

- Imported from the play-tested project state for publication.
- Uses a looping timer to attempt enemy spawns and exposes `SpawnRate = 1`, `MaxEnemiesAlive = 10`, and `SpawnDistanceFromPlayer = 1000` defaults.
- Tracks the live count through the enemy-death notification path so defeated enemies can be replaced.

---

## BH-004: Implement Player Damage, Invulnerability Window, And Run End

**Status:** `Done`
**Priority:** High  
**Milestone:** M2: Survival Loop  
**Assets:** `Content/Blueprints/Characters/BP_Player.uasset`, `Content/Blueprints/Enemies/BP_Enemy.uasset`

### Goal

Make enemy contact a meaningful failure condition by consuming player health and ending the run at zero.

### Research Required

- Decide whether `Health` remains owned by `BP_Player` or moves into a reusable health component before adding more damageable actors.
- Establish the enemy contact event path and verify repeated overlaps do not deal damage every frame.
- Define a minimal post-hit invulnerability duration and feedback mechanism needed to make contact behavior debuggable.
- Choose initial run-end behavior: pause/freeze with text, restart level action, or transition to a game-over widget.

### Scope

- Use the existing player `Health` variable or replace it with a clearly defined current/max-health pair.
- Handle valid enemy contact damage.
- Add a short invulnerability/cooldown guard after damage.
- Stop active gameplay or present a restart state when player health reaches zero.
- Provide basic visible or logged hit confirmation while UI work is pending.

### Out Of Scope

- Armor, regeneration, pickups, revive systems, meta progression, full pause menus, or persistence.

### Acceptance Criteria

- Contact with an enemy reduces health by a known amount.
- Sustained overlap respects the configured damage cooldown.
- Health reaching zero reliably ends the run or transitions to the chosen game-over state.
- The player cannot continue firing or being damaged indefinitely after run end.

### Validation

- Walk into one enemy and confirm cooldown behavior.
- Die while multiple enemies overlap and verify the run-end path executes once.

### Implementation Record

- Completed on 2026-05-26.
- Retained player-owned `Health` and added `ContactDamageInterval = 0.75`, `IsInvulnerable`, and `IsGameOver` state to `BP_Player`.
- Added a readable `ProcessContactDamage` function: it reads `ContactDamage` from the overlapping `BP_Enemy`, applies one hit per active interval, clears its timer after overlap ends, and clamps lethal health to zero.
- Added player overlap entry flow that starts damage processing only when no invulnerability window is active, preventing multiple simultaneous enemy overlaps from stacking immediate hits.
- Configured the enemy capsule to overlap the `Pawn` channel so contact events occur without blocking player movement.
- On lethal damage, the player clears both contact-damage and projectile-firing timers, marks the run over, and pauses gameplay.
- Compiled modified Blueprints with warnings treated as errors. In PIE, a controlled `100`-second interval test held player health at `990` during continued multi-enemy overlap after the initial `10`-damage hit; restored production defaults then reached `Health = 0` and `IsGameOver = true` at the `0.75`-second interval.

---

## BH-005: Add Minimal Gameplay HUD And Debug Metrics

**Status:** `Ready`
**Priority:** Medium  
**Milestone:** M3: Readable POC  
**Assets:** New or existing UI Blueprint assets, game mode/player as data sources

### Goal

Expose enough state to judge whether the survival loop works without inspecting Blueprint variables during play.

### Research Required

- Determine whether existing template UI assets are reusable or whether a small dedicated widget is clearer.
- Choose data ownership for elapsed time, defeated enemy count, and live enemy count so the HUD reads stable values.
- Decide whether updates should use event dispatchers or periodic binding for the small POC scope.

### Scope

- Display player current/max health.
- Display elapsed run time.
- Display kill count and optionally live enemy count as a diagnostic metric.
- Display a minimal game-over message/restart prompt if not already covered by `BH-004`.

### Out Of Scope

- Upgrade selection, experience bar, minimap, settings, styling pass, animation polish, or accessibility menu work.

### Acceptance Criteria

- During play, health reflects received damage.
- Kill count increments on enemy death.
- Elapsed time increases until run end.
- Game-over state is readable without opening editor debugging views.

### Validation

- Play through spawning, kills, damage, and death while watching each displayed value.

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

**Status:** `Blocked` by `BH-001` through `BH-006`  
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
| Pending | Pending | Pending | Establish after M3 is complete. |

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
| 1 | `BH-001` | Converts spawned shots into observable combat interactions. |
| 2 | `BH-002` | Produces the smallest complete kill loop with one enemy. |
| 3 | `BH-003` | Turns the kill loop into ongoing survival pressure and supplies a target registry. |
| 4 | `BH-004` | Adds player risk and a meaningful run-ending condition. |
| 5 | `BH-005` | Makes the loop observable and suitable for tuning. |
| 6 | `BH-006` | Prevents the current targeting strategy from defining the horde ceiling. |
| 7 | `BH-007` | Establishes code ownership before player behavior expands substantially. This decision may be executed earlier if C++ gameplay work begins. |
| 8 | `BH-008` | Defines the POC completion gate after the loop exists. |
| 9 | `BH-009` | Establishes the measured population ceiling and identifies the first actual bottleneck. |
| 10 | `BH-010` | Removes unnecessary hidden damage-text updates from every live enemy. |
| 11 | `BH-011` | Reduces a likely high-cost per-enemy world-widget burden. |
| 12 | `BH-012` | Addresses persistent per-enemy Tick, movement, and animation costs based on measured evidence. |
| 13 | `BH-013` | Selects the architecture required only if the target is several hundred enemies. |

## Deferred Backlog

These features are intentionally excluded from the initial proof-of-concept tasks and should not be started until `BH-008` provides evidence that the core loop is sound:

- Experience drops, leveling, and upgrade selection.
- Multiple weapons or projectile behaviors.
- Enemy archetypes, bosses, and authored wave schedules.
- Object pooling and deeper optimization.
- High-population enemy architecture implementation beyond the validated `BH-013` prototype.
- Audio, VFX, animation, and presentation polish.
- Persistence, meta progression, and packaged-build concerns.
