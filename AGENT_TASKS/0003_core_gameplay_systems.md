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

## Not yet functionally confirmed (needs a live PIE pass from the user)

- Does the HUD actually appear and update as health/stamina change?
- Does the placed `BP_EnemyBasic` actually chase and melee the player, and
  does the player's health bar visibly drop?
- Does sprinting actually drain stamina and get cut off at zero?
- Does headshot/falloff damage feel right (numbers are a first guess, not
  tuned against actual playtesting)?
