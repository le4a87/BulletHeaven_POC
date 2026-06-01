# Bullet Heaven POC Implementation History

This document preserves completed task summaries that have been rolled out of `TASKBOARD.md`. The task board should stay focused on pending and active work.

## Completed Gameplay Loop Tasks

### BH-001: Implement Functional Projectile

- Completed on 2026-05-23.
- Added a `CollisionSphere` query-overlap root and a visible sphere mesh child to `BP_Projectile`.
- Configured straight planar projectile motion at speed `1200` with gravity disabled.
- Added editable `Damage = 10` and finite `InitialLifeSpan = 3` defaults.
- Added the hit path `OnComponentBeginOverlap (CollisionSphere)` -> `Cast To BP_Enemy` -> `Apply Damage` -> `Destroy Actor`.
- Compiled with warnings treated as errors and saved the Blueprint.
- Ran PIE with the placed enemy present; live projectiles were observed through runtime actor inspection and no Blueprint runtime errors were reported.
- Actual enemy health reduction/death was completed in `BH-002`, where `BP_Enemy` consumes the standard damage event.

### BH-002: Implement Enemy Health, Pursuit, And Death

- Completed on 2026-05-23.
- Selected direct `CharacterMovement` pursuit for the POC rather than navigation, avoiding NavMesh dependencies before timed spawning work.
- Added editable `MaxHealth = 30`, `MoveSpeed = 250`, and `ContactDamage = 10`, plus runtime `CurrentHealth`, `PlayerTarget`, and `IsDead` state.
- Added `OnEnemyDied` as an event dispatcher for future counters, experience, and spawn tracking.
- Initialized health and cached a valid `BP_Player` target in `BeginPlay`; valid-target guarded Tick movement uses `Add Movement Input`.
- Added `Event AnyDamage` processing, single-death guarding, death dispatch, and actor destruction on lethal projectile damage.
- Compiled with warnings treated as errors and ran PIE with the placed enemy; after the multi-hit firing window the enemy was destroyed with no Blueprint/script runtime errors logged.

### BH-003: Implement Timed Enemy Spawning And Enemy Registry

- Imported from the play-tested project state for publication.
- Uses a looping timer to attempt enemy spawns and exposes `SpawnRate = 1`, `MaxEnemiesAlive = 10`, and `SpawnDistanceFromPlayer = 1000` defaults.
- Tracks the live count through the enemy-death notification path so defeated enemies can be replaced.

### BH-004: Implement Player Damage, Invulnerability Window, And Run End

- Completed on 2026-05-26.
- Retained player-owned `Health` and added `ContactDamageInterval = 0.75`, `IsInvulnerable`, and `IsGameOver` state to `BP_Player`.
- Added a readable `ProcessContactDamage` function: it reads `ContactDamage` from the overlapping `BP_Enemy`, applies one hit per active interval, clears its timer after overlap ends, and clamps lethal health to zero.
- Added an explicit player health-bar refresh after each contact-damage health write so the world-space player bar reflects damage before the run pauses.
- Added player overlap entry flow that starts damage processing only when no invulnerability window is active, preventing multiple simultaneous enemy overlaps from stacking immediate hits.
- Configured the enemy capsule to overlap the `Pawn` channel so contact events occur without blocking player movement.
- Restricted enemy pursuit facing to yaw-only rotation so close contact does not pitch characters onto the ground.
- On lethal damage, the player clears both contact-damage and projectile-firing timers, marks the run over, and pauses gameplay.
- Compiled modified Blueprints with warnings treated as errors. In PIE, a controlled `100`-second interval test held player health at `990` during continued multi-enemy overlap after the initial `10`-damage hit; restored production defaults then reached `Health = 0` and `IsGameOver = true` at the `0.75`-second interval.

### BH-005: Add Minimal Gameplay HUD And Debug Metrics

- Completed on 2026-05-28.
- Added native `BHGameplayHUD`, a minimal `AHUD` overlay for the prototype survival loop.
- Displays player health, elapsed run time, defeated enemies, live enemies, and a game-over message.
- Reads existing Blueprint-owned player variables by name so the HUD does not require broad gameplay ownership changes for this POC task.
- Caches live-enemy scans at a `0.25`-second interval to avoid adding per-frame actor queries while still keeping the diagnostic count responsive.
- Configured `BP_TopDownGameMode` to use `BHGameplayHUD` through its `HUDClass` default.
- Because `BHGameplayHUD` is a new native C++ class, already-running editor sessions must be restarted after the build before the class can be selected or used by PIE.
- Built `BulletHeavenPOCEditor` successfully with the new native HUD class.
- Ran `CompileAllBlueprints -WarningsAsErrors`; completed with `0` errors, `0` warnings, and `0` failed Blueprint loads.

### BH-006: Replace Repeated Global Target Scans

- Completed on 2026-05-29.
- Added native `BHEnemyRegistrySubsystem` to track live `BP_Enemy` actors for systems that need target candidates without querying every matching actor from repeated gameplay paths.
- Added `BHTargetingLibrary` Blueprint-callable helpers for nearest registered enemy, live enemy count, and defeated enemy count.
- Rewired `BP_Player.FireProjectile` from `GetAllActorsOfClass -> ForEachLoop -> distance comparison` to `FindNearestRegisteredEnemy -> Is Valid -> SpawnActor`.
- Removed the old nearest-enemy local-variable assignment chain from the firing graph; the returned target actor now feeds both validity checking and projectile aim location directly.
- Updated `BHGameplayHUD` to read live and defeated enemy counts from the registry instead of maintaining its own recurring actor scan.
- Validated the saved `FireProjectile` graph with commandlet inspection; the firing path no longer contains `GetAllActorsOfClass`, `For Each Loop`, `Set NearestEnemy`, or `Get NearestEnemy`.
- Built `BulletHeavenPOCEditor` successfully after the native targeting changes.
- Ran `CompileAllBlueprints -WarningsAsErrors`; completed with `0` errors, `0` warnings, and `0` failed Blueprint loads.
- Dedicated population measurement remains in `BH-009`; this task removes the per-shot global scan that would otherwise distort those results.

### BH-007: Decide And Apply Player Blueprint Ownership Strategy

- Completed on 2026-05-29.
- Chose to keep `BP_Player` parented directly to `Character` for the current proof of concept.
- `BP_Player` already owns the active camera, spring arm, health bar widget, health state, auto-fire timer, projectile spawning function, and Blueprint gameplay variables used by the HUD and combat loop.
- `ABulletHeavenPOCCharacter` remains an abstract template-derived native character shell with camera and spring-arm components, planar movement defaults, and stubbed `BeginPlay`/`Tick`; reparenting now would introduce duplicate component/default risk without moving meaningful gameplay ownership into C++.
- Native gameplay work should stay in shared services and read-only helpers for this POC, such as `BHGameplayHUD`, `BHEnemyRegistrySubsystem`, and `BHTargetingLibrary`, unless a future task deliberately migrates player behavior.
- `BP_TopDownGameMode` continues to spawn `BP_Player` and use the existing player controller/HUD setup.
- Validated by inspecting the active Blueprint class references and rebuilding `BulletHeavenPOCEditor`; no Blueprint reparent or asset save was required.

### BH-008: Establish POC Play-Test And Performance Gate

- Completed on 2026-05-29.
- Added `docs/POC_PLAYTEST_GATE.md` as the repeatable proof-of-concept acceptance gate.
- Defined a `5`-minute-or-game-over Play In Editor run from `Map_BulletHeavenPOC`.
- Captured the baseline parameters to preserve during the gate: player health `100`, fire rate `0.5`, contact-damage interval `0.75`, camera spring arm length `1650`, projectile damage `10`, projectile lifespan `3`, enemy health `30`, enemy move speed `250`, enemy contact damage `10`, spawner rate `1`, live enemy cap `10`, and spawn distance `1000`.
- Defined required observations for movement, automatic firing, repeated spawning, enemy pursuit, player damage, enemy death, HUD readability, projectile/enemy cleanup, and game-over behavior.
- Defined the performance capture using `stat fps`, `stat unit`, `stat game`, and `stat blueprint`, with pass criteria targeting playable editor behavior and no sustained game-thread time above `33.3 ms`.
- Added failure-handling guidance so follow-up tasks should be created only for observed gate failures.
- Linked the gate from `README.md`.
- Manual PIE validation was completed after restarting Unreal Editor on 2026-05-29; tester reported that everything seemed fine, with no observed gameplay failure from the defined gate.

### BH-014: Zoom Gameplay Camera Out Slightly

- Completed on 2026-05-29.
- Increased `BP_Player.SpringArm.TargetArmLength` from `1400` to `1650` for a modest wider top-down combat view.
- Kept the existing camera angle unchanged at approximately `Pitch = -50`, `Yaw = 45`, with spring-arm collision testing still disabled.
- Left movement, firing, HUD ownership, cursor behavior, and camera component setup unchanged.
- Compiled and saved `BP_Player` through the Unreal editor commandlet.
- Verified the saved Blueprint default by reading back `target_arm_length = 1650.0`; `Map_BulletHeavenPOC` map check reported `0` errors and `0` warnings during the commandlet run.
- Manual visual PIE play-test is still recommended to confirm combat readability, clipping, and crowded enemy approach feel on the live editor viewport.

### BH-015: Reuse Player Walking Animation On Enemy

- Completed on 2026-05-29.
- Confirmed `BP_Player.Mesh` and `BP_Enemy.Mesh` both use `SKM_Manny_Simple` with skeleton `SK_Mannequin`, making the player animation Blueprint compatible with the enemy mesh.
- Assigned `BP_Enemy.Mesh.AnimClass` to the same player animation Blueprint generated class, `/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C`.
- Added red enemy material instances `/Game/Characters/Mannequins/Materials/Manny/MI_Enemy_01` and `/Game/Characters/Mannequins/Materials/Manny/MI_Enemy_02`, then assigned them to `BP_Enemy.Mesh` slots 0 and 1 so enemies remain visually distinct from the player.
- Left enemy mesh, transform, collision, pursuit, damage, death, health bar, and damage text behavior unchanged.
- Compiled and saved `BP_Enemy` through the Unreal editor commandlet; the two new material instances were saved and asset validation ran on all three changed assets.
- Manual PIE validation after restarting Unreal Editor confirmed enemies use the expected walking animation and appear red.

### BH-016: Improve HUD Timer And Kill Stat Visibility

- Completed on 2026-06-01.
- Updated native `BHGameplayHUD` with viewport-aware panel margins and width so the runtime overlay stays readable across common PIE viewport sizes.
- Kept timer and kill count as the primary HUD group with larger values, stronger contrast, and consistent spacing.
- Preserved health, live-enemy count, and game-over messaging; health now also has a compact high-contrast bar inside the panel.
- Replaced fixed game-over text offsets with measured centered text placement.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`.
- Manual PIE validation on 2026-06-01 confirmed the HUD readability looked good.

### BH-010: Gate Damage-Text Billboard And Fade Updates

- Completed on 2026-05-29.
- Moved visible floating damage-number ownership into `BHHealthFeedbackSubsystem` instead of relying on each enemy's legacy `DamageText` presentation path.
- Added damaged-actor tracking for active native floating damage numbers so repeated damage on the same actor reuses one transient `ATextRenderActor`.
- Repeated hits now refresh the existing text actor's text, location, color, and lifetime instead of accumulating overlapping text actors during rapid hits.
- Damage-number update work returns immediately when no floating numbers are active, avoiding camera lookup and per-number billboard processing during the hidden steady state.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload` after the native feedback changes.
- Initial PIE validation confirmed damage numbers still appeared but exposed duplicate signed values from the old Blueprint presentation path; that presentation issue was resolved by the completed feedback fix below.

### BH-011: Reduce Per-Enemy Health-Bar Widget Cost

- Completed on 2026-06-01.
- Chose a damaged-only visibility policy for enemy world-space health bars so large groups do not maintain visible/ticking UMG widgets throughout combat.
- Added native widget visibility management to `BHHealthFeedbackSubsystem`; enemy actors with `CurrentHealth`/`MaxHealth` now have widget components hidden and unticked by default.
- On enemy damage, the subsystem shows that actor's health bar for `2.5` seconds, then hides and unticks it again.
- Kept player health feedback unaffected by limiting the policy to actors that expose `CurrentHealth` and `MaxHealth`.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`.
- Manual PIE validation on 2026-06-01 confirmed the damaged-only health-bar behavior looked good.
- After validation, `BP_EnemySpawner.MaxEnemiesAlive` was manually raised from `50` to `75` for current population testing; formal `BH-009` profiling remains pending.

### BH-012: Reduce Per-Enemy Tick And Character Cost

- Completed on 2026-06-01.
- Added native `BHEnemyUpdateBudgetSubsystem`, active only on `Map_BulletHeavenPOC`, to apply distance-based update policies to `BP_Enemy` instances without editing the Blueprint graph.
- Nearby enemies within `2000 cm` keep their original actor, movement, mesh, and animation tick settings.
- Mid-distance enemies use `0.05 s` actor tick, `0.033 s` movement tick, `0.05 s` mesh tick, and `OnlyTickPoseWhenRendered`; far enemies use `0.12 s` actor tick, `0.08 s` movement tick, `0.15 s` mesh tick, and `OnlyTickPoseWhenRendered`.
- Added per-enemy skeletal animation rate variation, defaulting to `0.94-1.06`, to break up identical walking gait synchronization without changing movement speed, pathing, collision, or damage pressure.
- Added console variable `bh.EnemyBudget.Enabled` for A/B profiling and documented the policy in `docs/BH-012_ENEMY_UPDATE_BUDGET.md`.
- Extended `BHPopulationTelemetrySubsystem` and `tools/compare_population_telemetry.py` with optional `BH-012` columns for budget-enabled samples, near/mid/far budget-band counts, and animation-rate variation count while keeping existing `BH-009` baseline CSVs readable.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`.
- Manual PIE validation on 2026-06-01 confirmed the update-budget and gait-variation pass looked good.
- Fixed-cap `100` telemetry comparison passed against baseline `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_150403.csv` using candidate `Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_152150.csv`.
- Candidate telemetry held cap `100` for `248` samples from `3.0-252.0s`, reached peak live enemies `100`, averaged `65.0 FPS`, recorded p95 average frame time `19.94 ms`, max one-second average frame time `20.55 ms`, worst frame `95.32 ms`, peak projectiles `6`, peak physical memory `4813.5 MB`, budget enabled for all `248` samples, peak budget bands `100/46/21`, and peak animation-rate varied enemies `100`.
- Compared to the cap-`100` baseline, sustained p95 average frame time improved from `20.65 ms` to `19.94 ms`, max one-second average frame time improved from `21.92 ms` to `20.55 ms`, and peak physical memory decreased from `4892.6 MB` to `4813.5 MB`; no telemetry regressions exceeded thresholds.

### BH-017: Scale Enemy Spawn Pressure Over Time

- Completed on 2026-06-01.
- Added native `BHSpawnPressureSubsystem`, active only on `Map_BulletHeavenPOC`, to drive the existing `BP_EnemySpawner` without rewriting its Blueprint graph.
- The subsystem discovers `BP_EnemySpawner`, writes the inspectable `MaxEnemiesAlive` and `SpawnRate` variables, and calls `TrySpawnEnemy` for supplemental pressure after the opening tier.
- The Blueprint spawner's existing cap check remains the guard against exceeding the active cap.
- Initial tier table is `0s: cap 75 / 1.00s / no supplemental`, `60s: cap 90 / 0.85s`, `120s: cap 110 / 0.70s`, `180s: cap 130 / 0.60s`, and `240s: cap 150 / 0.50s`.
- Supplemental native spawn attempts begin at the 60-second tier so short editor smoke tests retain the current opening pressure.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`.
- Manual PIE validation on 2026-06-01 confirmed the spawn pressure ramp looked good.

### BH-019: Add Randomized Cube Obstacles To Combat Map

- Completed on 2026-06-01.
- Added native `BHObstacleFieldSubsystem`, which spawns a bounded runtime field of cube blockers when `Map_BulletHeavenPOC` begins play.
- Defaults are `18` obstacles, `2300` cm placement radius, `750` cm player safe radius, `500` cm spawner safe radius, `520` cm minimum spacing, `280` cm footprint, and `240` cm height.
- The subsystem supports random layouts by default and a deterministic seed mode through config properties.
- Runtime cubes use `/Engine/BasicShapes/Cube`, block all collision channels, disable overlap generation, and opt out of character step-up.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`.
- No map asset was saved because the obstacle field is spawned at runtime by the subsystem.
- Manual PIE validation on 2026-06-01 confirmed cubes properly obstruct enemies and enemies path around them as expected.
- PIE validation also found that player targeting still selects obstructed enemies and projectiles pass through cubes; follow-up task `BH-020` tracks those combat-obstruction fixes.

### BH-020: Make Obstacles Block Targeting And Projectiles

- Completed on 2026-06-01.
- Added line-of-sight filtering to `BHEnemyRegistrySubsystem::FindNearestEnemy`; candidate enemies are rejected when a `Visibility` trace at combat height hits an actor tagged `BHObstacle`.
- The targeting trace ignores the player/source actor and candidate enemy, so an unobstructed enemy remains targetable while an obstacle-blocked enemy is skipped.
- Extended `BHObstacleFieldSubsystem` into a tickable subsystem that tracks `BP_Projectile` movement segments.
- Added projectile-obstacle traces on the `Visibility` channel; when a projectile movement segment crosses a `BHObstacle`, the projectile is destroyed without applying enemy damage.
- Preserved existing enemy projectile damage behavior for unobstructed shots.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`.
- Manual PIE validation on 2026-06-01 confirmed the targeting and projectile obstruction behavior looked good.

### BH-018: Prevent Enemy Stacking And Excessive Overlap

- Completed on 2026-05-29.
- Enabled built-in RVO avoidance on `BP_Enemy.CharacterMovement`, with `AvoidanceConsiderationRadius = 250` and `AvoidanceWeight = 1.0`.
- Set `BP_Enemy.SpawnCollisionHandlingMethod` to `AdjustIfPossibleButDontSpawnIfColliding` so enemy spawns can adjust away from immediate collision instead of starting in severe overlap.
- After PIE showed RVO alone still allowed visible clustering, added centralized native enemy separation to `BHEnemyRegistrySubsystem`.
- The registry now applies a bounded 2D separation pass over registered live enemies, pushing actors apart only within a `140` cm radius and clamping each actor correction to `60` cm per tick.
- Kept separation out of per-enemy Blueprint Tick and avoided repeated `GetAllActors` or per-enemy neighbor scans.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload` after the native separation change.
- `CompileAllBlueprints -WarningsAsErrors` reported `0` errors, `0` warnings, and `0` failed Blueprint loads; the commandlet's nonzero exit was caused by the editor MCP HTTP listener port already being bound by the open editor session.
- Manual PIE validation after restarting Unreal Editor confirmed the enemy clustering fix looked good.

## Completed Feedback Fixes

### Remove Duplicate Debug-Line Health Bars

- Completed on 2026-05-29.
- Removed `DrawDebugLine` health-bar rendering from `BHHealthFeedbackSubsystem`; those world-space debug lines were appearing as diagonal green/black bars over actors and duplicated the intended UMG health bars.
- Kept floating damage numbers active, billboarded toward the camera, short-lived, and faded through the native feedback subsystem.
- Reduced health-actor discovery from every frame to a `0.25`-second scan interval so steady-state feedback work is limited to active damage numbers.
- Built `BulletHeavenPOCEditor` successfully after the change.

### Display One Unsigned Damage Number Per Collision

- Completed on 2026-05-29.
- Identified the duplicate damage numbers as two presentation owners showing the same hit: `BP_Enemy`'s legacy `DamageText` component and the native `BHHealthFeedbackSubsystem` floating text actor.
- Updated `BHHealthFeedbackSubsystem` to force legacy `DamageText` components hidden on registered health-bearing actors and to own the visible floating damage number.
- Formatted native damage text with the absolute damage amount and no `+` or `-` prefix.
- Kept the camera-facing, short-lived, fading native presentation while preserving the one-active-number-per-damaged-actor reuse from `BH-010`.
- Increased `BP_EnemySpawner.SpawnDistanceFromPlayer` from `1000` to `2400` because the previous radius could place spawns inside the current `1650` spring-arm camera view.
- Preserved the manually tested `BP_EnemySpawner.MaxEnemiesAlive = 50` population cap.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload`; compiled and saved `BP_EnemySpawner` after the spawn-distance update.
- Manual PIE validation after restarting Unreal Editor confirmed the final damage-number and enemy-spawn presentation looked correct.
