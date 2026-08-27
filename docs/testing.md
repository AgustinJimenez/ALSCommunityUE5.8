# Automated Testing

How UE5 actually does automated gameplay testing, why it sidesteps the MCP's
PIE-world blind spot (see `mcp-notes.md`), and how it's wired up in this
project. **Implemented** as of the session that added health/stamina/enemy
AI (see `AGENT_TASKS/0003_core_gameplay_systems.md`) - CQTest and
CQTestEnhancedInput are enabled, `ALSHost.Build.cs` depends on them, and
`Source/ALSHost/Private/Tests/` has real passing tests
(`ALSHealthComponentTests.cpp`, `ALSStaminaComponentTests.cpp`,
`ALSEnemyAIControllerTests.cpp`). The investigation/plan below is kept for
context; see "What actually got built" further down for the current state.

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

## What actually got built (steps 1-2 done differently, 3-6 not needed yet)

Steps 1-2 above (enable `CQTest`/`CQTestEnhancedInput`, add to
`ALSHost.Build.cs`) were done exactly as planned - both are real engine
modules/plugins present even in a launcher/binary engine install (their
`Source/.../Private` folders are stripped, like almost every other engine
module, but the precompiled `Engine/Binaries/Win64/UnrealEditor-CQTest*.dll`
files link fine, the same as `Core`/`UMG`/every other engine dependency this
project already used without its source present).

Rather than the `Fire()`-specific plan in steps 3-6, the first real tests
covered `UALSHealthComponent`, `UALSStaminaComponent`, and
`AALSEnemyAIController` instead (built in the same session, needed
verification more urgently). Two things the plan didn't anticipate, found by
actually building it:

- **`FActorTestSpawner` does NOT work for anything BeginPlay-dependent.**
  Its own header says "no PIE loaded" and this is real, not a formality -
  confirmed directly that `World::HasBegunPlay()` and
  `Actor::HasActorBegunPlay()` are both false for actors spawned through it,
  so `UALSHealthComponent::BeginPlay()` (which sets `CurrentHealth =
  MaxHealth` and binds `OnTakeAnyDamage`) never ran, and every health
  assertion read 0. Use `FMapTestSpawner::CreateFromTempLevel(TestCommandBuilder)`
  instead for anything that needs real actor lifecycle - it stands up an
  actual (if minimal) PIE-equivalent world. It also requires an explicit
  `Spawner->AddWaitUntilLoadedCommand(TestRunner)` call right after creation
  (a hard assert fires otherwise: "Must call AddWaitUntilLoadedCommand in
  BEFORE_TEST") and `TestRunner` inside a `TEST_CLASS` body refers to the
  static `TTestRunner*` pointer, not the `FAutomationTestBase&` reference
  member of the same name it shadows - pass it directly, not `&TestRunner`.
- **`UGameplayStatics::GetPlayerPawn(this, 0)`** (what `AALSEnemyAIController`
  uses to find its target) **resolves through the world's first local
  player's `PlayerController`**, not just any `APlayerController` that
  happens to exist. A temp PIE world from `FMapTestSpawner` already has one
  local player (possessing whatever default pawn it auto-spawned) -
  spawning a second, disconnected `APlayerController` and calling `Possess`
  on it does nothing for `GetPlayerPawn`. Fetch the existing one instead
  (`UGameplayStatics::GetPlayerController(&Spawner->GetWorld(), 0)`) and
  `Possess()` your own test character with *that*.
- Assertion member functions live on `this->Assert` (the `FNoDiscardAsserter`
  the `ASSERT_THAT` macro expands against), not as free functions - despite
  `CQTestCondition.h` defining a similarly-named free `IsNearlyEqual`, the
  actual member to call for float comparisons is `IsNear(Expected, Actual,
  Epsilon)`.
- `FActorTestSpawner` (no PIE) is still exactly the right tool for pure
  construction/property-level checks that don't depend on BeginPlay/Tick -
  cheaper and faster than spinning up a whole temp world.
- Spawning an actor directly on top of another overlapping one (e.g. testing
  a pickup by spawning it at the same location as a character) is not just a
  test convenience issue - `OnComponentBeginOverlap` fires *synchronously,
  from inside the `SpawnActor` call itself* when the new actor spawns
  already overlapping something. This surfaced a real production bug (see
  `AGENT_TASKS/0003_core_gameplay_systems.md`'s pickup-actor section):
  calling `Destroy()` from that overlap handler makes `SpawnActor` return
  `nullptr` to its own caller. It also means any properties you mean to set
  on a freshly-spawned overlapping actor via the reference `SpawnActor`/
  `SpawnActorAt` returns must be set *before* whatever would trigger the
  overlap has a chance to read them - if the overlap already fired inline,
  setting `Pickup.SomeProperty = X` after the call is too late, the overlap
  handler already ran against the class defaults. Spawn away from the
  target and `SetActorLocation(..., bSweep=true)` into it once configured,
  both to sidestep the timing issue and because it's a more accurate
  simulation of "walking up to a dropped item" anyway.
- **There is no stock `"Projectile"` collision profile in vanilla UE5** -
  checked `Engine/Config/BaseEngine.ini` directly, it simply isn't defined.
  `SetCollisionProfileName(TEXT("Projectile"))` on a fresh component fails
  silently (no compile/runtime error, collision just stays unconfigured) -
  found this via a projectile test that flew straight through its target
  with zero hits; logging the projectile's own location every tick showed
  it sailing past a target sitting well within its path. Set
  `CollisionEnabled`/`ObjectType`/per-channel responses explicitly instead
  of trusting a profile name for anything project-specific like this.

**Update: the original `Fire()`/`FInputTestActions` plan is done too**
(`ALSWeaponFireInputTests.cpp`) - fires the weapon through the actual
`IA_Fire` Enhanced Input action via `FInputTestActions`' real input-injection
API, not a direct C++ call to `Fire()`, and passed on the first attempt once
the pattern from the other tests was in place: `FMapTestSpawner`'s temp
world already has a real local player's `PlayerController` with a fully
initialized Enhanced Input subsystem (`UGameplayStatics::GetPlayerController`
+ re-`Possess()`, same as `ALSEnemyAIControllerTests`), which is exactly
what `FInputTestActions` needs to inject into. `Shooter->SetOverlayState(EALSOverlayState::Rifle)`
before injecting input equips the rifle (runs `ALS_CharacterBP`'s real
`OnUpdateHeldObject` Blueprint event) so `Fire()` has a weapon mesh and
ammo to work with, exactly like real gameplay.

Not yet covered: enemy chase/pathfinding (`AAIController::MoveToActor`
needs a built `NavMesh`, which a bare `FMapTestSpawner` temp level doesn't
have - only the distance-gated attack logic that doesn't call `MoveToActor`
is covered; see `ALSEnemyAIControllerTests.cpp`'s file comment). Reachable
with the patterns established here if needed later (would need either a
real test map with baked navigation, or building nav data programmatically
in test setup).

## Running the tests

```
"<EngineDir>/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "<ProjectDir>/ALSHost.uproject" \
  -ExecCmds="Automation RunTests ALSHost;Quit" -unattended -nopause -nosplash -nullrhi -log
```

Then check `Saved/Logs/ALSHost.log` for `Test Completed. Result={Success|Fail}`
lines (grep for `Test Completed|Error:` - the two harmless
`LogAutomationTest: Error: Condition failed` lines that print at engine-init
time, timestamp `[  0]`, are unrelated pre-existing engine self-checks, not
this project's tests). `-nullrhi` skips GPU/rendering entirely, which is
fine since nothing here asserts on visuals. Narrow to one group with e.g.
`Automation RunTests ALSHost.Combat` instead of the bare `ALSHost` prefix.
