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

## Completed Feedback Fixes

### Remove Duplicate Debug-Line Health Bars

- Completed on 2026-05-29.
- Removed `DrawDebugLine` health-bar rendering from `BHHealthFeedbackSubsystem`; those world-space debug lines were appearing as diagonal green/black bars over actors and duplicated the intended UMG health bars.
- Kept floating damage numbers active, billboarded toward the camera, short-lived, and faded through the native feedback subsystem.
- Reduced health-actor discovery from every frame to a `0.25`-second scan interval so steady-state feedback work is limited to active damage numbers.
- Built `BulletHeavenPOCEditor` successfully after the change.
