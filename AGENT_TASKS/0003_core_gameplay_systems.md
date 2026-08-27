# 0003 - Core gameplay systems: health, stamina, inventory, HUD, basic enemy, weapon realism

Built autonomously while the user was AFK, per their explicit request to keep
going without stopping to ask questions and to verify everything statically
since they couldn't test live in the moment. Everything below compiled
cleanly and was re-verified via read_class_defaults/read_component_properties
after each editor rebuild+relaunch cycle (the project's known
unsaved-content-change-lost-on-force-kill gotcha - see AGENTS.md).

## What's done

**UALSHealthComponent** (`Source/ALSHost/Public+Private/Combat/ALSHealthComponent`)
- Generic damage/health tracking, hooks `AActor::OnTakeAnyDamage` rather than
  requiring a bespoke damage-application call, so anything already routed
  through `UGameplayStatics::ApplyDamage`/`ApplyPointDamage`/`ApplyRadialDamage`
  works with it for free.
- `MaxHealth`, optional slow regen (`RegenPerSecond`, off by default,
  `RegenDelaySeconds` after last damage), `OnHealthChanged`/`OnDeath`
  BlueprintAssignable delegates, `Heal()`, `ResetHealth()`.
- Added to `ALS_CharacterBP` (player) and `BP_EnemyBasic` (enemy).

**UALSStaminaComponent** (`Source/ALSHost/Public+Private/Stats/ALSStaminaComponent`)
- Drains while `AALSBaseCharacter::GetGait() == EALSGait::Sprinting` (the
  *actual* computed gait, already gated by ALS's own `CanSprint()` rules -
  e.g. aiming), regenerates otherwise after a short delay.
- Forces the character out of a sprint via `SetDesiredGait(Running)` when
  stamina hits zero - an external Tick-based gate, doesn't touch vendored
  ALS C++, same pattern `UALSWeaponFireComponent` already uses.
- Added to `ALS_CharacterBP` only (enemies don't sprint in this pass).

**UALSInventoryComponent** (`Source/ALSHost/Public+Private/Inventory/ALSInventoryComponent`)
- Minimal generic stack-based inventory (`FALSInventoryItem`: ItemID/
  DisplayName/Quantity/MaxStack), `AddItem`/`RemoveItem`/`GetItemQuantity`/
  `HasItem`, `OnInventoryChanged` delegate.
- No item data assets or pickup actors yet - there's no item content in the
  project to define them against. This is the foundation; next step is
  either a `UDataAsset`-based item definition table or just more `AddItem`
  call sites (e.g. a health-pack pickup actor calling `Heal()` +
  `RemoveItem`), and a UI list widget (skipped this pass - no in-game way to
  view it yet beyond `GetItems()`/Blueprint).
- Added to `ALS_CharacterBP` only.
- **Update**: now has real content - see "Pickup actors" below.

**AALSPickupBase / AALSItemPickup / AALSHealthPickup** (`Source/ALSHost/Public+Private/Inventory/`)
- Added in a later pass (after CQTest existed) specifically to give
  `UALSInventoryComponent` something to interact with. `AALSPickupBase` is
  overlap-triggered (a `USphereComponent` on the `"OverlapAllDynamic"`
  collision profile), subclasses only implement `OnPickedUp(APawn*) -> bool`;
  `AALSItemPickup` calls `AddItem` on the pawn's inventory, `AALSHealthPickup`
  calls `Heal` on its `UALSHealthComponent` (refuses if already full or dead,
  leaving the pickup in the world rather than consuming it for nothing).
  Two test instances (`ALSHealthPickup_0`, `ALSItemPickup_0`) placed next to
  the player start in `ALS_DemoLevel`.
- **Found and fixed a real bug this uncovered, not just a test artifact**:
  a pickup that spawns already overlapping a pawn (dropped on top of one, or
  spawned there deliberately) fires `OnComponentBeginOverlap` *synchronously
  from inside `SpawnActor` itself*, still on the stack below it. The first
  version called `Destroy()` directly from that overlap handler, which made
  `SpawnActor` return `nullptr` to its own caller instead of the actor it had
  just created - anything holding onto that return value (our own
  `spawn_actor`/`SpawnActorAt` callers included) would silently get a null
  pointer. Fixed by disabling the trigger collision and hiding the actor
  immediately, then `SetLifeSpan(0.01f)` to defer the actual `Destroy()` off
  the stack instead of calling it inline. Caught by CQTest hard-crashing
  with `Assertion failed: Actor != nullptr` the first time a test spawned a
  pickup directly on a character - see `docs/testing.md` for how that was
  root-caused (several dead-end hypotheses - spawn collision handling,
  double-firing overlaps - before finding the real cause) and
  `ALSPickupTests.cpp` for the now-passing tests, which also had to stop
  configuring pickup properties (`ItemID`, `HealAmount`, ...) on the
  reference `SpawnActorAt` returns *after* spawning it overlapping a
  character - same root cause, those property-set lines were running after
  the synchronous overlap had already fired and consumed the pickup using
  default values. Fixed by spawning away from the character, configuring,
  then `SetActorLocation(..., bSweep=true)` to move it into the character
  and trigger the overlap for real, once configured - which is also just a
  more realistic simulation of "walking into a dropped pickup" anyway.

**UALSHUDComponent + UALSStatusBarsWidget** (`Source/ALSHost/Public+Private/UI/`)
- `UALSHUDComponent` owns creating/showing a HUD widget, only for a locally-
  controlled pawn (retries on `ReceiveControllerChangedDelegate` for the same
  BeginPlay-before-possession race `UALSWeaponFireComponent`'s input binding
  already had to handle).
- `UALSStatusBarsWidget` finds `UALSHealthComponent`/`UALSStaminaComponent` on
  `GetOwningPlayerPawn()` itself (same pattern as `UALSDebugModesMenuWidget`)
  and binds to their changed-delegates - no manual wiring needed beyond
  pointing `HUDComponent->StatusBarsWidgetClass` at the WBP.
- Content: `WBP_StatusBars` (reparented to `UALSStatusBarsWidget`) - a
  `SizeBox` anchored bottom-left containing a `VerticalBox` with
  Health/Stamina `TextBlock`+`ProgressBar` pairs (red/green fill colors).

**AALSEnemyAIController + BP_EnemyBasic** (`Source/ALSHost/Public+Private/AI/ALSEnemyAIController`)
- Plain Tick-based chase+melee-attack FSM, not a Behavior Tree.
  **ClaudeUnrealMCP has `create_behavior_tree`/`create_blackboard`/
  `read_behavior_tree` but no tool to add composite/task/decorator nodes
  into a tree** - a real chase-vs-attack BT needs that and it doesn't exist
  yet. Went with C++ instead of blocking on building that tool given
  everything else in scope this pass; revisit per the project's usual
  "hit a gap, build the tool" philosophy if a proper BT-driven version is
  wanted (would need to investigate `FBehaviorTreeEditor`'s node-graph API,
  the same shape of work as `add_socket`/`retarget_anim_asset` were).
- Logic: acquires the player pawn via `UGameplayStatics::GetPlayerPawn`,
  `MoveToActor` when within `SightRange`, stops and calls
  `UGameplayStatics::ApplyDamage` on a cooldown (`AttackIntervalSeconds`)
  when within `AttackRange`, checks both its own and the target's
  `UALSHealthComponent::IsDead()` to stop acting once either side is dead.
- `BP_EnemyBasic` (`/Game/ALSHost/Characters/BP_EnemyBasic`) is a duplicate
  of ALS-Community's own `ALS_AIBP` (not a modification of the original
  vendored demo asset - duplicated via `run_python`'s `duplicate_asset` op
  so the crowd-NPC demo actors are untouched), reparented `AIControllerClass`
  to `AALSEnemyAIController`, with `UALSHealthComponent` added. Reuses
  `ALS_AIBP`'s already-correct ALS mesh/animation setup rather than building
  a new character class from scratch, since a plain `ACharacter` wouldn't
  animate correctly against `ALS_AnimBP` (its logic assumes an
  `AALSBaseCharacter` owner throughout).
- One instance (`TestEnemy_Basic1`, actor name `BP_EnemyBasic_C_0`) placed
  in `ALS_DemoLevel` near `PlayerStart` at `(-800, -1310, 92)` for immediate
  testing - not yet confirmed working live, only statically verified
  (compiles, components/AIControllerClass persisted, actor placed and level
  saved).
- Not done: no attack animation/montage (the AI controller applies damage
  directly, no visual attack), no ranged/shooting enemy variant, no
  patrol/idle-until-sighted state (currently always tries to acquire the
  player immediately - fine for one enemy, would need a real sight/perception
  check for stealth-relevant behavior).

**Weapon realism** (`UALSWeaponFireComponent`, `Source/ALSHost/.../Weapon/`)
- `Fire()` previously only logged hits, never applied any damage at all -
  now calls `UGameplayStatics::ApplyPointDamage` on hit, which reaches
  `UALSHealthComponent` via its `OnTakeAnyDamage` hook with zero coupling
  between the two systems.
- Added distance-based damage falloff (`DamageFalloffStartRange`,
  `MinDamageMultiplier` - linear falloff from full damage down to
  `MinDamageMultiplier` between `DamageFalloffStartRange` and `MaxRange`)
  and a headshot multiplier (`HeadshotMultiplier`, matched against
  `FHitResult::BoneName == HeadBoneName`, default `"head"` per
  ALS_Mannequin_Skeleton/stock Epic Manny naming).
- Not done: still a hitscan trace, not a real spawned projectile actor with
  travel time/drop/penetration - that's a materially bigger change
  (spawning an `AActor` with `UProjectileMovementComponent` per shot, network
  considerations, tracer VFX) and was judged lower value than the systems
  above for this pass given no projectile-specific visual assets exist in
  the project yet to make travel time visually read anyway. Revisit if the
  user specifically wants bullet drop/travel time to be a gameplay factor
  (e.g. for a sniper weapon).

## Verification performed (no live PIE access - see AGENTS.md on that gap)

- `Build.bat ALSHostEditor` succeeded with zero errors after each change
  (had to add `AIModule`/`GameplayTasks` to `ALSHost.Build.cs` for
  `AAIController` - missed on the first pass, caught by the linker errors).
- `compile_blueprint` on every touched Blueprint reports 0 errors/0 warnings.
- `read_class_defaults`/`read_component_properties`/`read_components`
  re-checked after the final rebuild+relaunch confirm every property/
  component survived (all four new components on `ALS_CharacterBP`,
  `HUDComponent.StatusBarsWidgetClass`, `BP_EnemyBasic.AIControllerClass`
  and its `HealthComponent`, the placed enemy actor and its position).
- MCP's TCP bridge reconnected on its own once the relaunched editor's
  listener came back up, without needing the user to run `/mcp` - polled
  `Test-NetConnection` on port 9877 rather than blocking on a fixed sleep.

## Update: automated gameplay tests built, most of the above now confirmed

After this was first written, `docs/testing.md`'s CQTest plan got
implemented (see that doc for the full writeup). Real, passing, headless
automated tests now confirm - not just "compiles", actual runtime behavior:

- `UALSHealthComponent`: starts at `MaxHealth`, damage reduces it correctly,
  lethal damage marks it dead and clamps at 0, damage after death doesn't go
  negative. (`ALSHealthComponentTests.cpp`, 5 tests)
- `UALSStaminaComponent`: starts at `MaxStamina`, actually drains while
  `EALSGait::Sprinting`. (`ALSStaminaComponentTests.cpp`, 2 tests)
- `AALSEnemyAIController`: an enemy within `AttackRange` of the player
  **does** damage them over time (the exact "does it chase and melee"
  question below, for the attack half - see caveat), an enemy beyond
  `SightRange` does not, and a dead enemy stops attacking.
  (`ALSEnemyAIControllerTests.cpp`, 3 tests)

Still not covered by automated tests (see `docs/testing.md`'s "not yet
covered" section for why): the actual chase/movement (`MoveToActor`) since
it needs a built NavMesh a synthetic test world doesn't have, whether the
HUD widget visually renders correctly (no visual assertions, `-nullrhi` skips
rendering), and whether headshot/falloff damage numbers feel right in
practice (tuning is a playtest question, not a correctness one - the math
itself isn't separately unit-tested yet either, worth adding). A live PIE
pass from the user is still the way to check those, but the core gameplay
logic - the part most likely to have an actual bug - now has real coverage
that will keep working on every future change, not just this one.
