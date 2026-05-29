# Bullet Heaven POC

An Unreal Engine proof of concept for a top-down survivor-style action game. The player moves through an arena while automatically firing at nearby enemies, surviving repeated enemy pressure, and receiving readable combat feedback.

This repository is an active prototype. It is intended for gameplay iteration and performance exploration rather than a packaged release.

## Current Features

- Top-down playable character and camera setup.
- Automatic targeting and projectile firing.
- Projectile collision and damage delivery.
- Enemies that pursue the player, receive damage, and die.
- Enemy contact damage with a short player invulnerability window and paused run-end state.
- Timed enemy-spawning setup with a configurable live-enemy cap.
- World-space enemy health and floating damage feedback.
- Billboarded, short-lived damage text for readability during combat.
- A task board for planned survival-loop and high-population scalability work.

## Project Layout

| Path | Purpose |
| --- | --- |
| `Content/Maps/Map_BulletHeavenPOC.umap` | Primary development/play-test map. |
| `Content/Blueprints/Characters/BP_Player.uasset` | Player gameplay behavior. |
| `Content/Blueprints/Enemies/BP_Enemy.uasset` | Enemy movement, damage, presentation, and death behavior. |
| `Content/Blueprints/Projectiles/BP_Projectile.uasset` | Base projectile behavior. |
| `Content/Blueprints/Spawners/BP_EnemySpawner.uasset` | Enemy population/spawn behavior. |
| `Content/Blueprints/UI/` | World health and damage feedback widgets. |
| `Source/BulletHeavenPOC/` | Native project and included template/variant code. |
| `TASKBOARD.md` | Roadmap, acceptance criteria, and performance follow-up tasks. |
| `docs/IMPLEMENTATION_HISTORY.md` | Completed task summaries and validation notes rolled out of the task board. |
| `AGENTS.md` | Blueprint cleanliness and validation conventions for automated contributions. |

## Requirements

- Unreal Engine **5.8** compatible editor/source build.
- A desktop target capable of Unreal Engine development. The project currently targets high-end desktop rendering settings.
- Git with [Git LFS](https://git-lfs.com/) installed. Unreal binary assets are stored through LFS.
- A C++ toolchain supported by your Unreal Engine installation:
  - Windows: Visual Studio 2022 with the Unreal/C++ workload, or equivalent supported setup.
  - Linux: the compiler/toolchain expected by your Unreal Engine source build.

The `.uproject` contains a local Unreal engine association. If it does not match your installation, associate the project with your local Unreal Engine 5.8 build when prompted.

## Getting Started

1. Clone the project and fetch LFS assets:

   ```bash
   git lfs install
   git clone https://github.com/le4a87/BulletHeaven_POC.git
   cd BulletHeaven_POC
   git lfs pull
   ```

2. Generate project files if required by your engine installation.

   Linux source build:

   ```bash
   /path/to/UnrealEngine/Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh \
     -project="$PWD/BulletHeavenPOC.uproject" -game -engine
   ```

   Windows source build:

   ```bat
   C:\path\to\UnrealEngine\GenerateProjectFiles.bat -project="%CD%\BulletHeavenPOC.uproject" -game -engine
   ```

3. Build the editor target if C++ binaries are not already available.

   Linux:

   ```bash
   /path/to/UnrealEngine/Engine/Build/BatchFiles/Linux/Build.sh \
     BulletHeavenPOCEditor Linux Development \
     -Project="$PWD/BulletHeavenPOC.uproject" -WaitMutex
   ```

   Windows:

   ```bat
   C:\path\to\UnrealEngine\Engine\Build\BatchFiles\Build.bat ^
     BulletHeavenPOCEditor Win64 Development ^
     -Project="%CD%\BulletHeavenPOC.uproject" -WaitMutex
   ```

4. Open `BulletHeavenPOC.uproject` in Unreal Editor.

5. Open `Content/Maps/Map_BulletHeavenPOC` and use Play In Editor to run the prototype.

## Packaging A Playable Build

Packaging has not been established as a release pipeline yet. For a local development package, use **Platforms > Package Project** in the Unreal Editor, or use Unreal Automation Tool after confirming the project builds in the editor.

Linux example:

```bash
/path/to/UnrealEngine/Engine/Build/BatchFiles/RunUAT.sh BuildCookRun \
  -project="$PWD/BulletHeavenPOC.uproject" -noP4 \
  -platform=Linux -clientconfig=Development \
  -build -cook -stage -pak -archive \
  -archivedirectory="$PWD/Dist/Linux"
```

Windows example:

```bat
C:\path\to\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat BuildCookRun ^
  -project="%CD%\BulletHeavenPOC.uproject" -noP4 ^
  -platform=Win64 -clientconfig=Development ^
  -build -cook -stage -pak -archive ^
  -archivedirectory="%CD%\Dist\Windows"
```

`Dist/`, packaged outputs, generated binaries, intermediate build files, editor saves, and derived data should not be committed.

## Development Notes

- The main gameplay implementation is currently Blueprint-driven.
- The project includes Unreal template content and source variants alongside the custom Bullet Heaven gameplay assets.
- Custom automation/editor integration plugins are intentionally not required by the checked-in project manifest; contributors can enable their preferred local tooling without making it a project dependency.
- Review [TASKBOARD.md](TASKBOARD.md) before expanding gameplay. It documents current milestones and planned performance work, including live-enemy scaling.
- Use [docs/POC_PLAYTEST_GATE.md](docs/POC_PLAYTEST_GATE.md) for the repeatable proof-of-concept play-test and performance gate.
- Review [docs/IMPLEMENTATION_HISTORY.md](docs/IMPLEMENTATION_HISTORY.md) for completed gameplay-loop and feedback implementation notes.
- Blueprint modifications should follow [AGENTS.md](AGENTS.md), including readable graph arrangement, avoiding unnecessary Tick work, compilation, saving, and practical validation.

## Performance Direction

The current enemy implementation is suitable for prototype gameplay but has known scaling work before supporting very large hordes. Planned work includes profiling at increasing enemy caps, gating transient damage-text updates, reducing per-enemy world-widget overhead, and evaluating lighter-weight enemy simulation for populations in the hundreds.

## Contributing

1. Create a branch for the change.
2. Keep Blueprint graph edits readable and scoped.
3. Compile and save all modified Blueprints.
4. Play-test affected gameplay in `Map_BulletHeavenPOC`.
5. Do not commit generated Unreal directories such as `Binaries/`, `Intermediate/`, `Saved/`, or `DerivedDataCache/`.
6. Include Git LFS objects for any added or updated Unreal binary assets.
