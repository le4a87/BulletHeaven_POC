# Project Instructions

## Blueprint Authoring Standards

When creating or modifying Blueprint graphs:

- Keep execution flow left-to-right with minimal wire crossings.
- Arrange all modified nodes before completing a task.
- Preserve clear vertical separation between independent event chains.
- Use functions or custom events when an event graph branch becomes conceptually distinct or difficult to scan.
- Prefer named functions over large inline event chains for reusable behavior or behavior with more than one responsibility.
- Keep pure/data-producing nodes close to the node that consumes them.
- Do not leave disconnected, unused, duplicate, or temporary debug nodes in modified graphs.
- Use comments only for non-obvious behavior boundaries or design constraints; do not comment trivial node sequences.
- Name variables, functions, dispatchers, and components according to their gameplay responsibility.

## Blueprint Performance Standards

- Avoid per-frame Blueprint Tick work unless the behavior must update every frame.
- Gate transient visual work so it executes only while visible or active.
- Avoid repeated `Get All Actors Of Class` calls in frequently executed gameplay paths.
- Consider cost growth when behavior is attached to every enemy, projectile, or world-space widget.

## Blueprint Completion Checklist

For every modified Blueprint:

- Arrange modified nodes into readable execution flow.
- Compile the Blueprint with warnings treated as errors where supported.
- Save the asset after successful compilation.
- Play-test affected behavior when practical.
- Report any graph cleanup intentionally deferred or any remaining performance concern.
