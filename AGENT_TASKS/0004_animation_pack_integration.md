# 0004 - Integrate Quaternius Universal Animation Library (packs 1 & 2)

## Status: RESOLVED

## Goal

User downloaded two free animation packs to `~/Downloads` and asked us to
check whether any of it is usable in ALSHost. After inspecting both, bring in
the animations that fill real, already-known gaps in this project (melee has
no swing animation at all, the Pistol has no reload/aim animation, there is
no hit-reaction feedback anywhere) - not the whole pack, just the pieces that
map to something this project actually needs.

This file is also meant as a template going forward: one `000N_slug.md` file
per non-trivial task, living in `AGENT_TASKS/`, updated as work actually
happens (not written once at the end) - status, what's been tried, what
worked, what didn't, and why. `AGENT_TASKS/0001-0003` already established
this pattern for earlier work; this file continues it.

## Source material found

- `~/Downloads/Universal Animation Library[Standard].zip` (UAL1) and
  `~/Downloads/Universal Animation Library 2[Standard].zip` (UAL2), by
  Quaternius (quaternius.com / patreon.com/quaternius).
- **License: CC0 1.0 Universal (public domain)** - confirmed by reading
  `License.txt` inside both zips directly, not assumed from the pack name.
  No attribution required, unrestricted use.
- Each zip ships the same animation set three ways: `Unity/*.fbx` (Unity
  Humanoid-style rig), `Unreal-Godot/*.glb`, and (UAL2 only) a separate
  `Female Mannequin` sub-pack. Each also comes in a plain version and an
  `_RM` (root motion baked in) version.
- Extracted to `E:\Temp\claude\anim_check\UAL1` / `UAL2` for inspection
  (scratch location, not committed).

## Skeleton compatibility - checked, not assumed

Imported `UAL1_Standard.fbx` (the Unity export, not even the Unreal-flagged
one) into Blender and read `armature.data.bones` directly. Bone names are
**already Epic Mannequin convention**: `root`, `pelvis`, `spine_01/02/03`,
`neck_01`, `Head`, `clavicle_l/r`, `upperarm_l/r`, `lowerarm_l/r`, `hand_l/r`
plus full finger chains (`index/middle/pinky/ring/thumb_0X_l/r`),
`thigh_l/r`, `calf_l/r`, `foot_l/r`, `ball_l/r`. This is the same bone
hierarchy AGENTS.md already documented for `ALS_Mannequin_Skeleton` when the
ResidentHorrorV1 reload-animation retargeting was done (see "On pulling
animations in from another project and retargeting them"). Expect the same
`retarget_anim_asset` pipeline to work directly, likely more cleanly than the
ResidentHorrorV1 case since these use Epic's own names already rather than a
merely-compatible rig.

## Full animation inventory (both packs, 43 each - listed for reference, not all of it is being used)

**UAL1**: A_TPose, Crouch_Fwd_Loop, Crouch_Idle_Loop, Dance_Loop, Death01,
Driving_Loop, Fixing_Kneeling, Hit_Chest, Hit_Head, Idle_Loop,
Idle_Talking_Loop, Idle_Torch_Loop, Interact, Jog_Fwd_Loop, Jump_Land,
Jump_Loop, Jump_Start, PickUp_Table, Pistol_Aim_Down, Pistol_Aim_Neutral,
Pistol_Aim_Up, Pistol_Idle_Loop, Pistol_Reload, Pistol_Shoot, Punch_Cross,
Punch_Jab, Push_Loop, Roll, Sitting_Enter, Sitting_Exit, Sitting_Idle_Loop,
Sitting_Talking_Loop, Spell_Simple_Enter/Exit/Idle_Loop/Shoot, Sprint_Loop,
Swim_Fwd_Loop, Swim_Idle_Loop, Sword_Attack, Sword_Idle, Walk_Formal_Loop,
Walk_Loop.

**UAL2**: A_TPose, Chest_Open, ClimbUp_1m, Consume, Farm_Harvest,
Farm_PlantSeed, Farm_Watering, Hit_Knockback, Idle_FoldArms_Loop,
Idle_Lantern_Loop, Idle_No_Loop, Idle_Rail_Call, Idle_Rail_Loop,
Idle_Shield_Break, Idle_Shield_Loop, Idle_TalkingPhone_Loop, LayToIdle,
Melee_Hook, Melee_Hook_Rec, NinjaJump_Idle_Loop/Land/Start, OverhandThrow,
Shield_Dash, Shield_OneShot, Slide_Exit/Loop/Start, Sword_Block, Sword_Dash,
Sword_Heavy_Combo, Sword_Regular_A/A_Rec/B/B_Rec/C/Combo, TreeChopping_Loop,
Walk_Carry_Loop, Yes, Zombie_Idle_Loop, Zombie_Scratch, Zombie_Walk_Fwd_Loop.

## What's actually being used, and why (the "all in" scope)

Not literally all 86 - farming/driving/spellcasting/zombie-NPC/climbing
animations have no matching system in this project (YAGNI - nothing to hang
them on). "All in" is scoped to every animation that fills a **real,
already-identified gap**:

1. **Melee swing animation** - `UALSMeleeComponent::TryMeleeAttack()`
   currently plays nothing at all, just an invisible sphere-sweep. Bringing
   in `Sword_Attack` (UAL1) for the axe/knife-equipped swing and
   `Punch_Cross`/`Punch_Jab` (UAL1) for the bare-fist swing.
2. **Pistol reload/aim animation** - only Rifle has one (the
   ResidentHorrorV1-retargeted `AS_Rifle_Reload`); Pistol's `Reload()` runs
   the timer with nothing playing. Bringing in `Pistol_Reload`,
   `Pistol_Aim_Down/Neutral/Up`, `Pistol_Shoot`, `Pistol_Idle_Loop` (all
   UAL1).
3. **Hit-reaction feedback** - does not exist anywhere in this project;
   taking damage has zero animated response. Bringing in `Hit_Chest`,
   `Hit_Head` (UAL1) and `Hit_Knockback` (UAL2), wired to
   `UALSHealthComponent::OnHealthChanged` (negative delta = took damage).

`Sword_Regular_A/B/C`/`Sword_Heavy_Combo`/`Sword_Block`/`Sword_Dash` (UAL2)
and `Death01` (UAL1) noted as candidates but deliberately deferred - a full
combo system and a pre-ragdoll death animation are each their own scoping
decision, not implied by "fix the missing swing animation" gap. Revisit
later if wanted.

## Plan

1. [x] Per animation: export a single-take FBX from Blender (armature + a
   generic body mesh + one action only) to a scratch path.
2. [x] Import each into Unreal as a full `AnimSequence`+`Skeleton`+
   `SkeletalMesh` (mirrors the ResidentHorrorV1 migration pipeline) into
   `/Game/ALSHost/Animations/Staging/`.
3. [ ] `retarget_anim_asset` each onto `ALS_Mannequin_Skeleton`, landing in
   `/Game/ALSHost/Animations/`, then rename to a clean `AS_<Name>` asset name.
4. [ ] Delete the `Staging/` folder once every retarget is confirmed
   independent (same check as the ResidentHorrorV1 migration -
   `get_dependencies` before deleting).
5. [ ] Wire the melee swing animations into `UALSMeleeComponent` (play via
   `PlaySlotAnimationAsDynamicMontage`, same mechanism `Reload()` already
   uses - pick fist vs weapon anim based on `HasAxeEquipped()`/
   `HasKnifeEquipped()`).
6. [ ] Wire Pistol reload/aim into `UALSWeaponFireComponent`'s
   `AmmoStatsByOverlayState[PistolOneHanded]` entry (`ReloadAnimation`, same
   field the Rifle entry already uses).
7. [ ] Build a small hit-reaction piece (new, doesn't exist yet) bound to
   `UALSHealthComponent::OnHealthChanged`, playing `Hit_Chest`/`Hit_Head`/
   `Hit_Knockback` on damage.
8. [ ] CQTest coverage wherever the CQTest/`-nullrhi` harness can actually
   observe it (montage-slot playback state, not visual correctness - see
   AGENTS.md's standing note on what this harness can and can't verify for
   UMG; the same caveat likely applies to AnimMontage playback, confirm
   rather than assume).
9. [ ] Full rebuild + headless test run, live verification via MCP, restart
   survival check for any new content assets.
10. [ ] `AGENTS.md` write-up once done; update this file's Progress Log to
    "RESOLVED" with a final summary.

## Progress Log

- **2026-08-29**: Packs located, license confirmed CC0, skeleton
  compatibility confirmed via direct Blender inspection, full inventory
  listed, scope decided (melee swing + pistol reload/aim + hit reactions).
  This file created.
- **2026-08-29**: All 12 selected animations exported one-per-FBX from
  Blender (armature + Quaternius's generic `Mannequin` body mesh + single
  baked action each, via `bake_anim_use_all_actions=False`) to
  `E:\Temp\claude\anim_check\export\`. All 12 imported into Unreal cleanly
  (`import_materials=False`/`import_textures=False`, matching the axe
  mesh-only-import crash workaround from the earlier Sketchfab-axe task -
  `Interchange.FeatureFlags.Import.FBX 0` set first) - zero crashes across
  12 imports, done in small batches (1-4 at a time via `execute_console_command`
  `py`, `ping`-checked between batches). Landed in
  `/Game/ALSHost/Animations/Staging/` as `AnimSequence`+`Skeleton`+
  `SkeletalMesh`+`PhysicsAsset` sets.
- **2026-08-29**: Started retargeting onto `ALS_Mannequin_Skeleton`.
  **`retarget_anim_asset` crashed the editor** partway through a batch of 4
  calls sent together (`Pistol_Idle_Loop`, then `Hit_Chest`/`Hit_Head`/
  `Hit_Knockback` never even got to run - `ECONNRESET` then
  `ECONNREFUSED`), after 7 of the 11 remaining retargets had already
  succeeded fine (`Sword_Attack` retargeted+renamed first, individually,
  before this batch). Not yet root-caused *why* `Pistol_Idle_Loop`
  specifically crashed it when 7 structurally-identical calls just before it
  didn't - worth checking if this is animation-specific (frame count/bone
  data) or purely a "one retarget too many in the same session" resource
  issue once the remaining ones are retried. Relaunching editor now;
  remaining to retarget: `Pistol_Idle_Loop`, `Hit_Chest`, `Hit_Head`,
  `Hit_Knockback` (UAL2). Succeeded so far (renamed or not yet - see next
  entry): `Sword_Attack`(renamed to `AS_Sword_Attack`), `Punch_Cross`,
  `Punch_Jab`, `Pistol_Reload`, `Pistol_Aim_Down`, `Pistol_Aim_Neutral`,
  `Pistol_Aim_Up`, `Pistol_Shoot` (these 7 still need the `AS_` rename pass).
- **2026-08-29 (continued)**: Two real, previously-undocumented issues found
  chasing the remaining 4 (`Pistol_Idle_Loop`, `Hit_Chest`, `Hit_Head`,
  `Hit_Knockback`):
  1. **A multi-asset FBX import's auxiliary packages aren't reliably flushed
     to disk even with `task.save = True`.** For a `SkeletalMesh` import that
     also generates a `Skeleton`+`PhysicsAsset`+`AnimSequence` as separate
     packages, only the primary `SkeletalMesh` package reliably lands on
     disk; the three auxiliary ones can exist only in-memory (visible to
     `search_assets`, invisible to a direct disk check) until something else
     flushes them. When the editor later crashed (unrelated cause, see next
     point) mid-session, all four of these animations' auxiliary packages
     were lost even though they'd "successfully imported" much earlier -
     only the bare `SkeletalMesh` `.uasset` remained on disk afterward, for
     all four, confirmed by direct `ls`/`find` on the `Content/` folder
     (`search_assets`, which reads the in-memory registry, could not be
     trusted to prove this - had to check the filesystem directly). Fix
     going forward: after any FBX import that produces multiple linked
     packages, explicitly call
     `unreal.EditorLoadingAndSavingUtils.save_dirty_packages(save_content_packages=True)`
     (or save each `task.imported_object_paths` entry individually) and,
     critically, **verify on disk** (not via `search_assets`) before trusting
     it, especially before anything that might force-kill or crash the
     editor.
  2. **Python (`execute_console_command`'s `py` path) can silently stop
     executing at all after an editor crash+relaunch, with zero error
     surfaced anywhere.** After the crash and a first relaunch, every `py`
     script I ran appeared to "execute" (`execute_console_command` returned
     `"executed": true`, and even the raw `Cmd: py\n<script>` line correctly
     echoed into `ALSHost.log`) but produced **no observable effect
     whatsoever** - no `LogPython:` output for a bare `print(...)`, and (the
     conclusive test) a plain `open(path, "w").write(...)` never created the
     file on disk at all. This cost real time: multiple reimport attempts
     for the same 4 animations appeared to do nothing or do something subtly
     wrong, and it was only traceable back to "Python isn't running at all"
     after deliberately testing a side effect independent of Unreal's own
     asset system (a raw file write) rather than trusting `execute_console_command`'s
     `"executed": true` or the presence of a `Cmd: py` log line - **neither
     one proves the script actually ran**, only that the command string was
     dispatched. Fix: a second, clean relaunch (not just retrying the same
     `py` call) resolved it - not yet root-caused *why* Python execution
     specifically breaks this way (worth a `read_log`/`obj dump` pass on the
     Python plugin's own state next time it recurs, rather than
     immediately relaunching, if there's time to investigate). Lesson for
     this project's standing "verify, don't trust a success response"
     discipline: it now needs to extend to **`execute_console_command`
     itself possibly no-oping silently** - after any batch of `py` calls
     whose success matters, sanity-check with a trivial, independently-
     observable side effect (a file write, not a `print`) before trusting a
     long batch actually ran, especially the first call after any editor
     crash/relaunch.
- **2026-08-29 (continued)**: Root cause narrowed further on the "Python
  silently does nothing" issue: still not fixed by a second clean relaunch
  either - a plain `open(path,'w').write(...)` and even
  `EditorLevelLibrary.spawn_actor_from_class` (checked via `find_actors_by_name`,
  a completely different verification path) both produced zero effect
  across two consecutive fresh boots. Non-Python console commands
  (`Interchange.FeatureFlags.Import.FBX 0`) kept working perfectly the whole
  time, proving `execute_console_command`'s dispatch mechanism itself was
  healthy - the breakage was specific to Python. Asked the user to look at
  the actual Editor window (matches this file's own established "modal
  dialog can silently block without hanging the process" pattern) - they
  reported the editor had crashed a third time and reopened on its own; a
  Python file-write immediately after that confirmed working again. Net
  cause still not fully pinned down (possibly a Python-side deadlock/wedge
  that only a genuine crash+restart clears, not just a clean relaunch) -
  worth deeper investigation only if it recurs.
- **2026-08-29 (continued)**: Redid the remaining 4 imports under fresh
  `_v2`-suffixed names (avoiding any stale-package collision from the
  repeated failed attempts) with `create_physics_asset=True` explicitly set
  and an explicit `save_dirty_packages` call, then verified all 16 expected
  files (4 anims x Mesh+Skeleton+PhysicsAsset+AnimSequence) directly via
  `find` on disk before proceeding - this time all 4 produced complete sets.
  All 12 animations retargeted onto `ALS_Mannequin_Skeleton` with zero
  further crashes (checked connectivity with `ping` after every single
  retarget call, not just at the end) and renamed to the `AS_<Name>`
  convention. **Found a real scope correction while wiring**: the project
  already had a working, wired-in `AS_Pistol_Reload` (dated before this
  session, from earlier work not fully reflected in this file's original
  gap analysis) - `PistolOneHanded`'s `AmmoStatsByOverlayState` entry
  already references it. Renamed the newly-retargeted duplicate to
  `AS_Pistol_Reload_UAL` to avoid a collision/overwrite and left the
  existing one untouched. **Descoping the Pistol aim/shoot/idle
  animations**: `FALSWeaponAmmoStats` has no field for an aim-pose or
  fire/idle animation at all (only `ReloadAnimation` exists) - wiring
  `AS_Pistol_Aim_Down/Neutral/Up`/`AS_Pistol_Shoot`/`AS_Pistol_Idle_Loop`
  would mean adding new fields and playback logic to
  `UALSWeaponFireComponent`, not just filling an existing empty slot. All 5
  are imported, retargeted, and available in `/Game/ALSHost/Animations/` for
  a future task, but wiring them is deliberately deferred here to keep this
  task's scope to what was actually planned (melee swing + hit reactions) -
  noting this the same way the Sword-combo/`Death01` deferrals were already
  noted above.
- **2026-08-29 (continued)**: Wired the two remaining pieces:
  - `UALSMeleeComponent` gained `FistSwingAnimation`/`WeaponSwingAnimation`/
    `MeleeMontageSlotName`, played via `PlaySlotAnimationAsDynamicMontage`
    (same call `Reload()` already uses) inside `TryMeleeAttack()`, before the
    hit-sweep so a miss still animates. `WeaponSwingAnimation` is used
    instead of `FistSwingAnimation` whenever `HasAxeEquipped()`/
    `HasKnifeEquipped()` is true.
  - New `UALSHitReactionComponent` - binds
    `UALSHealthComponent::OnHealthChanged`, plays a random entry from
    `HitReactionAnimations` whenever `Delta < 0` (damage, not healing) and
    the character isn't already dead. No per-bone hit-location data reaches
    `OnHealthChanged`, so it can't distinguish a head hit from a body hit
    yet - noted as a real, scoped-out refinement, not silently ignored.
  - Both covered by new CQTest cases (`ALSInteractionTests.cpp`'s
    `MeleeComponent_Attack_PlaysSwingMontage`,
    `ALSHitReactionComponentTests.cpp`'s `TakingDamage_PlaysHitReactionMontage`
    /`HealingAtFullHealth_DoesNotPlayHitReactionMontage`) - all three passed
    on the first run, which **answers the open question from step 8 of the
    plan**: `UAnimInstance::GetCurrentActiveMontage()` genuinely is
    observable under this project's `-nullrhi` CQTest harness, unlike the
    UMG `NativeConstruct` case documented elsewhere in `AGENTS.md` - montage
    *state* isn't gated behind Slate/rendering the way widget construction
    is. Full suite: 81/81 passing.
- **2026-08-29 (final)**: Wired the retargeted animations onto the live
  Blueprints: `ALS_CharacterBP`'s `MeleeComponent` got
  `FistSwingAnimation=AS_Punch_Cross`, `WeaponSwingAnimation=AS_Sword_Attack`;
  both `ALS_CharacterBP` and `BP_EnemyBasic` gained a new
  `HitReactionComponent` with `HitReactionAnimations=[AS_Hit_Chest,
  AS_Hit_Head, AS_Hit_Knockback]` (array-of-object-references set via
  `AnimSequence'/Path.Asset'` ImportText syntax inside a parenthesized list -
  worked first try, unlike the earlier single-object pin case that needed
  a real C++ fix). Verified every property read back correctly *and*
  survived a full force-kill + relaunch cycle (not just a save response) -
  `check_all_blueprints` clean throughout. `Staging/` folder (25 leftover
  intermediate packages) deleted via a scripted
  `EditorAssetLibrary.delete_asset` loop, one straggler needing an
  individual retry. Final animation set in `/Game/ALSHost/Animations/`:
  `AS_Punch_Cross`, `AS_Punch_Jab`, `AS_Sword_Attack` (wired - melee swings),
  `AS_Hit_Chest`, `AS_Hit_Head`, `AS_Hit_Knockback` (wired - hit reactions),
  `AS_Pistol_Aim_Down/Neutral/Up`, `AS_Pistol_Shoot`, `AS_Pistol_Idle_Loop`,
  `AS_Pistol_Reload_UAL` (imported + retargeted, deliberately not wired -
  see the descoping note above; `FALSWeaponAmmoStats` has nowhere to plug
  an aim-pose/fire-animation in yet).

**Summary for anyone picking this up later**: melee attacks (fist and
axe/knife) now play a real swing montage instead of nothing; taking damage
now plays a random hit-reaction montage instead of nothing. Both are new,
genuinely-missing pieces of feedback this project didn't have before. Five
Pistol-specific animations are sitting in the project, already correctly
retargeted onto `ALS_Mannequin_Skeleton`, ready for whoever adds aim-pose/
fire-animation support to `UALSWeaponFireComponent` next - that's real
follow-on work, not part of this task's scope. Two Sword-combo sets
(`Sword_Regular_A/B/C`/`Combo`, `Sword_Heavy_Combo`/`Block`/`Dash` in UAL2)
and `Death01` were identified but never even imported - noted as candidates
if a real combo system or a pre-ragdoll death animation is ever wanted.
