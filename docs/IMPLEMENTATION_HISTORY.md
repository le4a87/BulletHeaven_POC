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

### BH-010: Gate Damage-Text Billboard And Fade Updates

- Completed on 2026-05-29.
- Moved visible floating damage-number ownership into `BHHealthFeedbackSubsystem` instead of relying on each enemy's legacy `DamageText` presentation path.
- Added damaged-actor tracking for active native floating damage numbers so repeated damage on the same actor reuses one transient `ATextRenderActor`.
- Repeated hits now refresh the existing text actor's text, location, color, and lifetime instead of accumulating overlapping text actors during rapid hits.
- Damage-number update work returns immediately when no floating numbers are active, avoiding camera lookup and per-number billboard processing during the hidden steady state.
- Built `BulletHeavenPOCEditor` successfully with `-NoHotReload` after the native feedback changes.
- Initial PIE validation confirmed damage numbers still appeared but exposed duplicate signed values from the old Blueprint presentation path; that presentation issue was resolved by the completed feedback fix below.

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
