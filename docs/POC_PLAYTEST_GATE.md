# Bullet Heaven POC Play-Test Gate

This gate defines the minimum repeatable test for deciding whether the current proof of concept is stable enough for population tuning and broader gameplay expansion.

## Gate Target

Run `Map_BulletHeavenPOC` in Play In Editor for `5` minutes or until game over, whichever happens first.

The run passes only if the prototype demonstrates the full survival loop without recurring Blueprint runtime errors, unbounded actor buildup, or obvious editor-frame stalls on the development machine.

## Baseline Parameters

Use the checked-in defaults unless the test row explicitly says otherwise.

| System | Parameter | Expected Baseline |
| --- | --- | --- |
| Map | Test map | `Content/Maps/Map_BulletHeavenPOC` |
| Player | Health | `100` |
| Player | Fire rate | `0.5` seconds |
| Player | Contact damage interval | `0.75` seconds |
| Player camera | Spring arm length | `1650` |
| Projectile | Damage | `10` |
| Projectile | Lifespan | `3` seconds |
| Enemy | Health | `30` |
| Enemy | Move speed | `250` |
| Enemy | Contact damage | `10` |
| Spawner | Spawn rate | `1` second |
| Spawner | Max enemies alive | `75` |
| Spawner | Spawn distance from player | `2400` |

## Setup

1. Build `BulletHeavenPOCEditor` if native source changed since the editor was opened.
2. Restart Unreal Editor after native builds so PIE uses the current module.
3. Open `Content/Maps/Map_BulletHeavenPOC`.
4. Use a fresh Play In Editor session, not Simulate.
5. Do not change Blueprint defaults unless the test row records the override.

## Required Observations

During the run, confirm these behaviors:

| Area | Pass Condition |
| --- | --- |
| Player control | The player can move throughout the test without camera or collision regressions. |
| Auto fire | Projectiles spawn repeatedly and aim at live enemies. |
| Enemy spawning | Enemies spawn repeatedly around the player up to the configured live cap. |
| Enemy pursuit | Enemies chase and can reach the player. |
| Damage loop | Enemies damage the player at the configured contact interval. |
| Enemy death | Projectiles damage and destroy enemies, and the kill counter increases. |
| HUD | Health, elapsed time, kills, live enemies, and game-over messaging are readable. |
| Cleanup | Projectiles expire or are destroyed on hit; defeated enemies do not remain counted as live. |
| Run end | If health reaches zero, gameplay pauses and the game-over state is visible. |

## Performance Capture

Enable these console stats while the run is active:

- `stat fps`
- `stat unit`
- `stat game`
- `stat blueprint`

Record approximate values rather than exact frame-by-frame logs:

| Metric | Record |
| --- | --- |
| Average FPS range | Typical visible FPS during active combat. |
| Worst observed FPS | Lowest sustained value, ignoring one-frame editor hitches. |
| Game thread | Typical and worst `stat unit` game-thread time. |
| Blueprint time | Typical and worst visible `stat blueprint` time. |
| Peak live enemies | Highest HUD live-enemy count observed. |
| Peak projectiles | Highest approximate simultaneous projectile count. |
| Runtime errors | Any recurring Blueprint/script/runtime errors from the output log. |

## Pass Criteria

The gate passes when all of these are true:

- The run lasts `5` minutes or reaches a valid game-over state.
- Automatic firing, enemy kills, repeated spawning, player damage, and game-over behavior all function.
- Live enemies stay bounded by the configured cap.
- Projectiles do not accumulate beyond their lifespan or hit behavior.
- No recurring Blueprint runtime errors appear.
- The editor remains broadly playable, targeting `60 FPS` when possible and accepting short dips only if the game thread does not stay above `33.3 ms`.

## Failure Handling

If the gate fails, record the failure in `TASKBOARD.md` and add a follow-up task only for the observed problem. Do not expand survivor features until the failure is understood.

## Result Template

| Date | Build/Configuration | Duration | Result | Peak Live Enemies | Peak Projectiles | FPS / Frame-Time Notes | Issues |
| --- | --- | --- | --- | --- | --- | --- | --- |
| YYYY-MM-DD | PIE, Development Editor | 5:00 or game-over time | Pass/Fail | Count | Approx count | `stat fps`, `stat unit`, `stat blueprint` summary | Runtime errors or gameplay failures |
