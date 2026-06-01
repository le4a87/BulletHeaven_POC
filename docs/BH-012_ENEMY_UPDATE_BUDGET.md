# BH-012 Enemy Update Budget

`BH-009` fixed-cap telemetry shows that the current full Blueprint `Character` enemy remains acceptable through cap `100`, but frame time rises with live enemy count. This first `BH-012` pass keeps the existing enemy Blueprint and applies conservative runtime budgeting from native code.

## Runtime Policy

`UBHEnemyUpdateBudgetSubsystem` is active only on `Map_BulletHeavenPOC` in PIE/game worlds.

| Distance From Player | Actor Tick | Character Movement Tick | Mesh Tick | Animation Policy |
| ---: | ---: | ---: | ---: | --- |
| `0-2000 cm` | Original Blueprint setting | Original component setting | Original component setting | Original mesh setting |
| `2000-4200 cm` | `0.05 s` | `0.033 s` | `0.05 s` | `OnlyTickPoseWhenRendered` |
| `>4200 cm` | `0.12 s` | `0.08 s` | `0.15 s` | `OnlyTickPoseWhenRendered` |

The subsystem caches each enemy's original settings on discovery and restores them when an enemy returns to the near band or when the subsystem shuts down. It does not change collision, damage, spawning, pathing goals, or enemy health behavior.

## Gait Variation

Each discovered enemy also receives a deterministic skeletal animation rate scale between `0.94` and `1.06`. This breaks up identical walking gait timing without changing movement speed, pathing, collision, or damage pressure.

## Telemetry

Population telemetry now appends these `BH-012` columns:

| Column | Meaning |
| --- | --- |
| `EnemyBudgetEnabled` | `1` when `bh.EnemyBudget.Enabled` is active for the target map. |
| `EnemyBudgetNear` | Enemies currently using original near-band update settings. |
| `EnemyBudgetMid` | Enemies currently using the mid-distance budget profile. |
| `EnemyBudgetFar` | Enemies currently using the far-distance budget profile. |
| `EnemyAnimRateVaried` | Enemies with per-instance animation rate variation applied. |

## Console Control

Use this before PIE to disable the budget pass for A/B profiling:

```text
bh.EnemyBudget.Enabled 0
```

Restore normal optimized behavior with:

```text
bh.EnemyBudget.Enabled 1
```

## Validation Plan

1. Play-test close-range swarms and enemies approaching around obstacles.
2. Run a fixed-cap `100` candidate trial with `bh.EnemyBudget.Enabled 1`.
3. Compare against the accepted cap-`100` baseline:

```bash
python3 tools/compare_population_telemetry.py \
  --baseline Saved/Profiling/BH009_UEDPIE_0_Map_BulletHeavenPOC_20260601_150403.csv \
  --candidate <new-candidate-csv>
```

If movement stutter or path-following regressions appear, widen the near band before considering deeper enemy architecture changes.
