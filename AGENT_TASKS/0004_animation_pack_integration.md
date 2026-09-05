# 0004 - Integrate Quaternius Universal Animation Library (packs 1 & 2)

## Current punch status (2026-09-05)

`AS_Punch_Cross` rebuilt from the Unreal/Godot GLB with consistent centimeter
units and a separate clean retargeter; see [0005](0005_punch_clean_retarget.md).
The older fixes below are investigation history, not the current recipe.
Other animations in this batch remain unverified.

## Original Status: RESOLVED

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

## CORRECTION (2026-08-30): the retargeted animations are broken - CQTest coverage never caught it

A completely separate later session wired left-click to trigger melee
whenever no ranged weapon is equipped (previously it only worked via the `V`
key). The very first live test - unarmed, clicking - made the player
character's mesh **visually vanish**, leaving only the (unrelated)
weapon-fire debug spheres and a camera snap. Bisected by temporarily
swapping `FistSwingAnimation` to the long-proven `AS_Rifle_Reload` - the
character stayed fully visible, isolating the cause to the animation asset
itself, not the new click-wiring or the melee sweep/damage logic.

Root cause, confirmed with `compare_anim_bone_pose` (sampling an animation
against *itself* at the same time, to read raw per-bone pose data rather
than compare two different animations): `AS_Punch_Cross`'s `pelvis`,
`hand_r`, and `foot_r` bones all return the **exact same world position** -
anatomically impossible, and the signature of a retarget that only
transferred the root/pelvis track while every other bone kept collapsed/
degenerate data. Checked further and found the same corruption in
`AS_Sword_Attack` (the weapon-swing animation) and, worse, `AS_Hit_Chest`
- which is actively wired into `UALSHitReactionComponent` on *both*
`ALS_CharacterBP` and `BP_EnemyBasic`, meaning this has almost certainly
been silently firing on every damage-taken event since this task originally
shipped, not just on the melee path that happened to surface it now. All 12
animations from this batch should be treated as suspect until individually
re-verified with the same bone-pose-sampling check - only 3 have actually
been confirmed broken so far (`AS_Punch_Cross`, `AS_Sword_Attack`,
`AS_Hit_Chest`); the rest (`AS_Punch_Jab`, `AS_Hit_Head`,
`AS_Hit_Knockback`, five unwired `AS_Pistol_*`) are simply unchecked, not
confirmed good.

**Why the original CQTest coverage (step 8 of the plan, "all three passed on
the first run") didn't catch this**: those tests only ever assert
`UAnimInstance::GetCurrentActiveMontage()` returns non-null after triggering
a swing/hit-reaction - a montage playing 100% garbage bone data satisfies
that check perfectly. "Montage became active" and "montage produces a valid
pose" are different claims; only the second one is the one that actually
matters. See the new, more detailed write-up in `AGENTS.md` ("On wiring
left-click to melee... discovering the entire Quaternius
animation-retargeting batch... was silently broken") for the full technique
and reasoning - worth reading before touching any of this again.

**Immediate mitigation applied, not a real fix**: `HitReactionAnimations`
cleared to empty on both characters (no animation now, rather than a broken
one); `FistSwingAnimation`/`WeaponSwingAnimation` on `MeleeComponent`
temporarily repointed at `AS_Rifle_Reload` as a working-but-visually-wrong
placeholder. **Still needed**: root-cause *why* the retarget dropped
non-root tracks for this specific batch (a Blender export setting? something
about how `EditorAnimUtils::RetargetAnimations()` handles these particular
source files?) and redo the retarget properly for all 12 animations, each
one verified with the bone-pose-sampling check before being wired back in
anywhere.

## ROOT CAUSE FOUND + AS_Punch_Cross fixed (2026-09-01)

Root cause is **not** a Blender export setting. Confirmed via
`compare_anim_bone_pose` self-sampling at three stages:

1. The source FBX (`UAL1_Punch_Cross.fbx`, re-imported fresh into a
   `Staging/` scratch package) has genuinely correct, distinct per-bone
   animation - `pelvis`/`hand_r`/`foot_r` land at physically sane, clearly
   different positions. **The source data was never broken.**
2. `retarget_anim_asset` (wraps `EditorAnimUtils::RetargetAnimations()`,
   `convert_space=true` default) reproducibly collapses every non-root bone
   to the destination skeleton's rest pose *specifically when the two
   skeletons' scale/proportions differ enough* - reproduced twice, once at
   the raw ~0.85-unit import scale and once after pre-scaling the import to
   ~85 units (matching `ALS_Mannequin_Skeleton`'s own ~90-unit pelvis
   height) - scaling the import didn't fix it, it just moved *where* the
   collapse point landed (confirming it's not a scale-tuning problem, it's
   this retarget path being the wrong tool for this proportion mismatch -
   matches the tool's own documented caveat).
3. `convert_space=false` avoids the collapse but leaves bone translations in
   a mismatched coordinate frame (pelvis stuck at source scale while other
   bones show large raw offsets) - not directly usable either.

**Real fix: a proper IK Retargeter setup**, built entirely via Python
(`unreal.IKRigController`, `unreal.IKRetargeterController`,
`unreal.IKRetargetBatchOperation`), which handles the proportion mismatch
correctly:

1. Create an `IKRigDefinition` for the source skeleton (the fresh
   Quaternius import) and one for `ALS_Mannequin_Skeleton`, each via
   `IKRigController.set_skeletal_mesh()` +
   `apply_auto_generated_retarget_definition()` - auto-generated chain
   names came out identical on both skeletons (`Spine`, `Neck`, `Head`,
   `Left/RightLeg`, `Left/RightFoot`, `Left/RightClavicle`,
   `Left/RightArm`, finger chains), so no manual chain mapping was needed.
2. Create an `IKRetargeter` asset, `IKRetargeterController.set_ik_rig()`
   for both `SOURCE`/`TARGET`, `add_default_ops()`,
   `assign_ik_rig_to_all_ops()`, `auto_map_chains(FUZZY, False)`.
3. Batch-retarget via `IKRetargetBatchOperation.run_batch_retarget()` (not
   the deprecated `duplicate_and_retarget` - same struct-based inputs,
   `assets_to_retarget` needs `AssetData` not loaded objects). **The
   output package is not flushed to disk automatically** - explicit
   `EditorAssetLibrary.save_asset()` needed, same "new asset not on disk
   yet" gotcha as multi-package FBX imports elsewhere in this project.
4. Verified with `compare_anim_bone_pose`: `hand_r` now swings a genuine
   ~55-unit arc with real rotation change across the punch, `pelvis`/
   `hand_r`/`foot_r` are no longer collapsed together.

**Result**: `AS_Punch_Cross` replaced in place with the correctly IK-
retargeted version (old broken one deleted first, had zero referencers
since it was never actually wired - `FistSwingAnimation` was still
pointing at the `AS_Rifle_Reload` placeholder). `MeleeComponent`'s
`FistSwingAnimation` repointed at the fixed `AS_Punch_Cross`, Blueprint
recompiled, full CQTest suite re-run headlessly - 100% pass, exit code 0,
including `MeleeComponent_Attack_PlaysSwingMontage`.

**Follow-up bug found on live test (2026-09-01, same day)**: the punch
looked right in isolated bone-pose checks but on the actual character it
snapped 180° with the hip dropping to the floor and legs spread out. Root
cause: the **Pelvis Motion op's `scale_horizontal`/`scale_vertical` default
to `1.0`** - the FK Chains op scales limb bones correctly via the target
skeleton's own bone lengths (rotation-only, scale-invariant), but the
Pelvis Motion op copies the pelvis's *translation* close to 1:1 from
source to target with no proportion correction unless this is set
explicitly. Result: pelvis stayed at the source skeleton's tiny raw scale
(~0.9 units) for the whole clip while the limbs were correctly scaled to
ALS proportions (~10-100 units) - a mesh with its pelvis pinned near the
origin but limbs stretched out to full-size positions looks exactly like a
snapped, spread-eagle collapse. Confirmed via `compare_anim_bone_pose`
sampled across the *entire* timeline (not just one frame) - the pelvis
track itself is smooth, so this isn't a mid-clip glitch, it's a constant
scale-mismatch between pelvis and limbs.

Note: `IKRigController.get_ref_pose_transform_of_bone('pelvis')` reported
a nonsensical ~9167 for the source rig (100x too large even after
refreshing the rig's skeletal mesh reference) - don't trust that accessor
for sanity-checking scale; `compare_anim_bone_pose` against the actual
retargeted `AnimSequence` is the reliable ground truth throughout this
whole investigation.

**Fix, take 1 (wrong, overcorrected)**: set `scale_horizontal =
scale_vertical = 100.0` uniformly on the Pelvis Motion op. This fixed the
vertical collapse but **overcorrected the horizontal (X/Y) axis** - the
punch's actual hip sway was already small and roughly correct at
`scale_horizontal=1.0`, so multiplying it by 100 too turned a ~1-unit sway
into a ~102-unit lurch, which on the live character reads as the pelvis
snapping ~1-2 meters forward with legs stretched taut (found on live
in-game test, not caught by `compare_anim_bone_pose` spot-checks alone -
worth sampling frame 0, i.e. the *rest-ish* pose, specifically, since a
constant baseline offset is invisible if you only ever diff two in-motion
frames against each other).

**Fix, take 2 (correct)**: `scale_horizontal=1.0` (left alone),
`scale_vertical=100.0` (the actual needed correction) - vertical and
horizontal needed independent, different corrections here, don't assume
one uniform scale factor covers both. Verified via `compare_anim_bone_pose`
at frame 0 (rest-ish pose): pelvis X/Y now near-zero as expected for a
standing punch, Z still correctly ~87-90; hand_r/foot_r unchanged from the
already-good take-1 values (only the pelvis op was touched). Saved into
the shared `IKRT_QuaterniusToALS` retargeter asset - future reuse (Sword
Attack, Hit Chest, etc.) should double check this split (don't assume
horizontal needs the same scale as vertical for those either; verify each
axis independently).

Swapped into `AS_Punch_Cross` the same way as before (temp-rename dance
needed *again* each time because `rename_asset`'s reference fixup
repoints existing Blueprint references to whatever the old asset gets
renamed *to* - repoint `FistSwingAnimation` back explicitly after both
renames every time, don't assume it survives untouched). Full CQTest
suite re-run after each swap - still 100% pass, exit code 0.

## Third bug found on live test: 180° facing flip (2026-09-01, same day)

Pelvis scale/translation was now correct, but the character still visibly
rotated 180° on the swing. Root-caused by comparing against a **known-good
reference animation** (`AS_Rifle_Reload`, never showed this bug) via
`compare_anim_bone_pose`: its `root` bone reads `yaw=0` (identity) at rest,
but the punch retarget's `root` bone was baked at a constant `yaw=-180` -
confirmed this is a genuine bug (not an ALS-Mannequin-convention quirk) by
having a real baseline to compare against, rather than guessing from the
punch data in isolation. `root` isn't part of any FK retarget chain (only
Spine/Neck/Head/limbs are), so nothing in the auto-generated op stack was
correcting its orientation - it was passed through close to raw from the
source skeleton's own root convention, which apparently faces backward
relative to Unreal's.

**Fix**: `IKRetargeterController.set_rotation_offset_for_retarget_pose_bone
('root', Rotator(yaw=180).quaternion(), RetargetSourceOrTarget.SOURCE)` -
a calibration offset stored in the retarget pose (not a per-op setting like
the scale fixes), canceling the 180° facing mismatch. Verified `root` now
reads `yaw=0` matching the known-good baseline; pelvis `roll` flipped by
~180° accordingly (expected - the whole downstream chain now includes the
correction), translation (X/Y near-zero, Z~87) unchanged from the earlier
fix. Saved into the shared `IKRT_QuaterniusToALS` retargeter. Re-swapped
into `AS_Punch_Cross`, re-wired `FistSwingAnimation`, full CQTest suite
re-run again - still 100% pass, exit code 0.

**Lesson for verifying any future retarget from this pipeline**: don't
just check that a bone's own motion is "smooth" or "distinct" in isolation
- diff it against a **known-good animation on the same target skeleton**
(any already-working `AS_*` asset) at the same bone, at rest. A systematic
offset (constant scale error, constant rotation flip) is invisible when
you only ever compare frames of the *same, suspect* clip against each
other - it needs an external reference point to show up.

## Fourth bug: left arm swung backward, and how it actually got fixed (2026-09-02)

With root/pelvis/facing all correct, the left arm (the non-punching one)
still looked wrong live - swung behind the torso instead of resting
naturally. `upperarm_l`/`lowerarm_l` bone lengths were confirmed correct
(matched the right arm exactly - `upperarm->lowerarm` ≈30.3cm,
`lowerarm->hand` ≈27.0cm on both sides), so this was purely a rotation/
orientation problem on the left arm chain, isolated from everything else
already fixed.

**What didn't work**: guessing a single rotation axis/angle in the dark
and re-running the full import+retarget+swap cycle each time to check via
raw bone-position numbers alone. Several rounds of yaw/pitch/roll offsets
on `upperarm_l` and `lowerarm_l` individually made it worse or just moved
the problem around, because reasoning about "is this bone's numeric
position now more forward/backward" from coordinates alone, without an
actual visual, is unreliable once a chain has multiple compounding
rotations.

**What actually worked - build a real visualization instead of guessing
blind**:
1. Sampled every bone's component-space position at a single frame
   (t=0.3s, mid-swing) via `compare_anim_bone_pose`, self-compared to
   extract a stable snapshot.
2. Built an interactive 3D pose viewer as an Artifact (Three.js, no
   library beyond the CDN UMD build) - skeleton as connected joints/
   lines, color-coded by limb (right arm = known-good reference, left
   arm = suspect), plus a translucent capsule "body mesh" toggle so the
   pose reads as an actual body instead of a wireframe, plus a facing-
   direction arrow (root's calibrated `yaw=0` means UE's `+X` is true
   forward - this exists specifically because "screen-left" is not
   "character's left" once you've free-orbited a 3D view; it flips
   depending on whether you're looking from the front or the back, same
   as a mirror), plus floating 2D labels pinned to each hand's actual
   3D position via `Vector3.project(camera)` (removes the same left/
   right ambiguity for the specific bones being debugged).
3. **Critical enabler: opened the artifact in a real, sighted browser via
   the `chrome-devtools` MCP** (`file://` on the local HTML, not the
   hosted artifact URL - that needs auth this browser doesn't have) and
   actually looked at rendered screenshots, instead of reasoning from
   coordinate numbers alone. This turned "guess an axis, wait ~30s for a
   full retarget cycle, check numbers, repeat" into "look at the picture,
   see it's still wrong which way, adjust."
4. With eyes on it, went through `clavicle_l` (the *top* of the chain,
   not individual arm bones - flipping the chain root reorients the whole
   limb as one unit) at `yaw=180` (made it worse - further backward),
   `roll=180` (overcorrected - arm ended up pointing straight up above
   the head), then `roll=90` (halfway between those two, since 0° and
   180° were opposite ends of the same rotation arc) - which read as a
   completely natural boxing-guard pose (elbow bent, hand near chest/
   chin height, roughly mirroring the punching arm) confirmed from two
   different camera angles.

**CORRECTION - `roll=90` was wrong, verified against only one frame**:
`clavicle_l roll=90` was checked at a single frame (t=0.3s) and looked
like a plausible boxing-guard pose there, so it got swapped in and
documented as the fix. Checking the *rest of the timeline* (t=0, t=0.6,
t=1.0) after the fact showed the hand swung dramatically behind the torso
at every other sampled point - the t=0.3 frame was a brief, coincidental
crossing point during the swing, not evidence the rotation was actually
correct. **A single-frame check cannot validate a chain rotation fix -
sample multiple points across the timeline before trusting a result,**
the same lesson as the earlier "verify against a rest frame, not just
in-motion frames" note but one level further: even a rest-*ish* frame
isn't enough on its own, check several.

**Further investigation once this was caught**: compared the *raw,
unretargeted source* animation's hand positions at t=0 - both hands sit
naturally forward/centered there, confirming the backward-swing is
introduced by retargeting, not inherited from source. Neither
`LOCAL_ROTATION_AXES` nor `GLOBAL_ROTATION_AXES` auto-align detects any
rest-pose misalignment on `clavicle_l` (returns identity both times),
meaning this isn't a simple pose-calibration problem to begin with.
Toggling the "Run IK Rig" op off entirely produced numerically **identical**
hand positions to leaving it on with identity offsets - it has no
measurable effect on this chain at all, ruling out a bad auto-generated
IK pole vector as the cause too.

**CORRECTION (2026-09-05, later session)**: re-tested this exact claim directly
and it's false, at least for the retargeter's current state - disabling the
"Run IK Rig" op with ALL bone offsets at identity changed `hand_l` at t=0.3
from `(-10.0, -14.3, 131.7)` to `(14.0, -28.0, 96.6)` - a ~35-unit drop in
height alone, nowhere close to identical. The op is doing real, necessary
work via its `LeftHandIK`/`RightHandIK` goals; don't disable it as a
troubleshooting step without re-verifying this first. Unclear whether the
2026-08-30 test was run against a different retargeter state or was simply
wrong; don't trust it without re-checking.

**Where this actually stands**: with all left-arm bone offsets reset to
identity (the state from before any of this sub-investigation, `Run IK
Rig` re-enabled), the left hand sits only mildly behind center
(~5-7cm, roughly constant across the whole timeline) - much closer to a
plausible guard stance than any of the large rotation guesses produced,
and identical to the very first swap-in before arm-specific tuning began.
This clean-baseline version is what's currently wired into
`AS_Punch_Cross`. Whether this baseline is actually correct, or whether
the original complaint ("elbow bends the wrong way / twisted") points at
a smaller, different problem (a roll/twist on the elbow itself, not a
gross positional offset) is **still open** - needs a real live look (PIE
+ screenshot, or the 3D pose Artifact sampled across multiple frames, not
one) before trying another correction.

**Standing lesson**: for any future retarget-chain rotation problem in
this project, build the 3D pose visualization and actually look at it via
`chrome-devtools` (or ask the user to look at the live PIE result) rather
than iterating blind on coordinate numbers - a chain of compounding
rotations is very hard to reason about from raw XYZ alone, especially
past 2-3 bones deep. And **always sample multiple points across the
animation's timeline, not one** - a single frame, even a deliberately-
chosen one, can coincidentally look right while the rest of the clip is
still wrong.

**Reusable infra kept** (not thrown away after use) at
`/Game/ALSHost/Animations/Retargeting/`: `IKRig_QuaterniusSource`,
`IKRig_ALSMannequin`, `IKRT_QuaterniusToALS`. Fixing `AS_Sword_Attack`,
`AS_Hit_Chest`, and the 9 still-unverified animations from this same batch
should reuse this retargeter rather than rebuilding it - just re-import
the relevant source FBX at `import_uniform_scale=100` (matches
`ALS_Mannequin_Skeleton`'s ~90-unit scale closely enough) and run
`IKRetargetBatchOperation.run_batch_retarget()` against it.
