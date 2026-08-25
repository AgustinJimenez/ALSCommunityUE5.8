# Automated Testing

How UE5 actually does automated gameplay testing, why it sidesteps the MCP's
PIE-world blind spot (see `mcp-notes.md`), and a concrete, scoped plan for
adding it to this project. Not implemented yet — this is the investigation
and plan, written down so it doesn't need re-doing.

## Why this exists

We tried to smoke-test `ALSWeaponFireComponent::Fire()` by calling it
directly on the live PIE player pawn via the MCP, to avoid needing a human
to manually click. Every actor-query tool available — `list_actors`,
`find_actors_by_name`, `get_scene_summary`, and raw Python
`GameplayStatics.get_all_actors_of_class(None, ...)` — turned out to be
scoped to the **editor world**, not the running **PIE world**
(`UEDPIE_0_ALS_DemoLevel`). The dynamically-spawned player pawn only exists
in the PIE copy and is invisible to all of them. See `mcp-notes.md` for the
full writeup of that dead end.

## What UE5 actually provides

Checked directly against the UE 5.8 engine source, not recalled from memory:

- **Classic Automation Spec tests** — `IMPLEMENT_SIMPLE_AUTOMATION_TEST` /
  `IMPLEMENT_COMPLEX_AUTOMATION_TEST` macros, defined in
  `Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h`. Pure C++ tests,
  no world required unless the test explicitly opens a map. Good for
  isolated logic (e.g. the spread-angle math), not for full gameplay
  scenarios.
- **Functional Tests** — `AFunctionalTest`, defined in
  `Engine/Source/Developer/FunctionalTesting/Classes/FunctionalTest.h`.
  Actors you place in a dedicated test level; they call `StartTest`, then
  `FinishTest(EFunctionalTestResult::Succeeded/Failed, Message)` when done.
  The classic "place a scenario in a level, run it, assert the outcome"
  pattern.
- **CQTest + CQTestEnhancedInput** — `Engine/Plugins/Tests/CQTest` and
  `Engine/Plugins/Tests/CQTestEnhancedInput`. Epic's newer GoogleTest-style
  framework. `CQTestEnhancedInput` specifically ships `FInputTestActions`
  and `FTestAction`
  (`CQTestEnhancedInput/Source/CQTestEnhancedInput/Public/Components/InputTestActions.h`),
  which **programmatically inject an Enhanced Input action** into a pawn via
  the engine's real input-injection API, not simulated hardware events, plus
  `FMapTestSpawner` to load a test map and find the spawned player pawn.
  This is the officially-supported way to test Enhanced-Input-driven
  gameplay, and matches our exact situation (an Enhanced Input action
  bound on a component, tested end to end).
- **Gauntlet** — `Engine/Plugins/Experimental/Gauntlet`. Full
  packaged-build, multi-client, device-farm end-to-end testing. Much
  heavier than anything needed here; not relevant at this project's size.

All of these run as code *compiled into the engine process itself* -
discovered and run via the editor's Session Frontend "Automation" window,
or headlessly for CI via
`UnrealEditor-Cmd.exe Project.uproject -ExecCmds="Automation RunTests <TestName>;Quit" -unattended -nopause`.
That is the structural reason they don't hit the problem we hit: the test's
own `GWorld` correctly follows into the PIE world because the test *is*
engine code, not an external client reaching in through a console command
after the fact.

## Plan for this project

Not yet done. Scoped, in order:

1. Enable the `CQTest` and `CQTestEnhancedInput` plugins in
   `ALSHost.uproject` (both ship with the engine - one-line entries, same
   pattern as every other plugin enabled so far).
2. Add `CQTest` and `CQTestEnhancedInput` to `ALSHost.Build.cs`'s
   dependencies.
3. Give `ALSWeaponFireComponent::Fire()` something a test can actually
   assert against. Right now it only logs and draws a debug line - add
   either an `OnWeaponFireHit` delegate (`DECLARE_DYNAMIC_MULTICAST_DELEGATE`)
   or a readable `LastFireHitResult` property, populated after every call.
4. Build a small, dedicated test map (not `ALS_DemoLevel` - that has 42 AI
   characters and a full landscape, unnecessarily slow and nondeterministic
   for a unit-style test). Minimal: a `PlayerStart`, the ALS character
   pawn, and a static target actor placed at a known, fixed distance in
   front of where the muzzle will be.
5. Write one `TEST_CLASS` following the documented pattern in
   `InputTestActions.h`'s header comment: `FMapTestSpawner` loads the test
   map, `FindFirstPlayerPawn()` gets the character, a custom `FTestAction`
   named `"IA_Fire"` gets injected via `FInputTestActions`, then assert
   `LastFireHitResult` (or the delegate firing) points at the known target
   actor.
6. Verify it runs both from the in-editor Automation window and headlessly
   via `-ExecCmds="Automation RunTests ..."`, since the headless path is
   what would actually matter for CI later.

Realistic sizing: comparable to the `ClaudeUnrealMCP` socket-authoring
tools work (a new small C++ addition, one rebuild-and-verify cycle) - not a
multi-day undertaking, but genuinely new territory (`FMapTestSpawner` and
`FInputTestActions` are unfamiliar), so expect some trial and error getting
the wiring right on the first attempt, the same as everything else
documented in `AGENTS.md`.
