# 0001 - Rifle reload animation: gun/hand offset tuning

## Goal

`AS_Rifle_Reload` (retargeted from ResidentHorrorV1 onto ALS's skeleton - see
AGENTS.md's "pulling animations in from another project" section) plays
correctly as an animation, but the M4A1 rifle's position/rotation relative to
the hands looks wrong while it plays, because the rifle is rigidly attached to
a fixed virtual bone (`VB RHS_ik_hand_gun`) with a static offset tuned for
ALS's own idle/aim/fire poses, not for a reload animation authored against a
different weapon's grip geometry in the source project. Need to find the
correct `ReloadHeldObjectLocationOffset` / `ReloadHeldObjectRotationOffset`
values (or determine that no constant offset can fix it) so the reload looks
right.

## What's done and working

- `UALSWeaponFireComponent` (Source/ALSHost) has full hitscan/full-auto/bloom/
  recoil/ammo/reload mechanics, verified live.
- `Reload()` plays `AS_Rifle_Reload` via `PlaySlotAnimationAsDynamicMontage`
  on the `"Grounded Slot"` AnimGraph slot, timed to finish exactly when ammo
  refills.
- `FALSWeaponAmmoStats` (per-overlay-state struct) has
  `ReloadHeldObjectLocationOffset` (FVector) and
  `ReloadHeldObjectRotationOffset` (FRotator), applied additively to
  `HeldObjectRoot` for the duration of `Reload()` and reverted in
  `FinishReload()`. **These are editable right now, directly in the
  `ALS_CharacterBP` -> `WeaponFireComponent` Details panel, under `Ammo Stats
  By Overlay State` -> `Rifle`, with zero dependency on the debug tool below.**
  This is the reliable fallback path if the debug tool keeps giving trouble.
- Camera zoom (mouse wheel, `AALSHostPlayerCameraManager`) - unrelated
  feature added in the same session, working.

## The debug tuning tool (built, live-update status unconfirmed as of last check)

- Hold **T** -> equips Rifle, freezes movement (camera look still works),
  shows mouse cursor, opens `WBP_RifleReloadTuning`, loops the reload
  animation. Release before ~2s = a tap, which instead just toggles the panel
  open/closed (old single-tap behavior, restored per user request). Holding
  past `DebugReloadHoldThresholdSeconds` (2s) toggles the anim loop on/off
  instead of closing the panel (`ToggleDebugReloadAnimLoop`).
- `WBP_RifleReloadTuning` is reparented to a C++ class,
  `UALSRifleReloadTuningWidget` (Source/ALSHost/Public+Private/UI) - see
  AGENTS.md's "debug reload-offset tuning tool's architecture" section for
  why (no generic Blueprint-graph node-authoring tool exists in
  ClaudeUnrealMCP yet). 6 sliders (loc X/Y/Z, rot pitch/yaw/roll), a "Copy
  Offsets to Clipboard" button, and a "Freeze Animation" button
  (`ToggleDebugReloadFreeze`, pauses the montage + the replay timer in place
  via `Montage_Pause`/`Montage_Resume`) were all added.
- **User-reported symptom, most recent test:** no numbers ever appear next to
  the sliders, dragging them does nothing, and the Freeze button does
  nothing either - i.e. the widget never seems to actually receive
  `TargetComponent`.
- **Diagnosis so far:** added temporary `UE_LOG` calls throughout
  `UALSRifleReloadTuningWidget` (SetTargetComponent, NativeConstruct, each
  OnXChanged handler) and confirmed via `read_log`/direct log read that
  `ALSWeaponFireComponent: started reload offset tuning` fires correctly
  (T works, PC is valid, mouse cursor toggles) but **none** of the widget's
  own log lines ever appear - meaning `CreateWidget()` never actually
  produced our subclass, or never ran at all. Root-caused (with reasonable
  but not 100%-confirmed confidence) to `set_component_property` never
  forcing a Blueprint recompile - see AGENTS.md for the full writeup. Called
  `compile_blueprint` on `ALS_CharacterBP` as the fix.
- **Not yet re-verified after the compile_blueprint fix** - that was the last
  action taken before the user asked to pause and write these notes instead.
  Next session should re-test T-hold in PIE and re-check the log for the
  widget's `UE_LOG` lines before doing anything else with this tool.
- The diagnostic `UE_LOG` calls are still in the code
  (`ALSRifleReloadTuningWidget.cpp`) - fine to leave for now, remove once the
  tool is confirmed working, so normal play doesn't spam the log.

## New/fixed ClaudeUnrealMCP capability, not yet committed or pushed

- Fixed: `add_widget` GUID-registration crash (hit adding a `Button`).
- Fixed: `set_component_property` corrupting `TSubclassOf<T>` (FClassProperty)
  properties, causing a delayed crash on save/serialize.
- Added: `set_widget_property` now falls back to the child widget's
  `UPanelSlot` (e.g. `HorizontalBoxSlot::Size`) when the property isn't found
  on the widget class itself - this is what let the tuning sliders actually
  fill their row width instead of using the tiny auto-sized default.
- Added: `compare_anim_bone_pose` - wraps
  `UAnimPoseExtensions::GetAnimPoseAtTime`/`GetBonePose` (not otherwise
  Blueprint/Python-exposed) to diff a bone's world-space transform between
  two animations at two given times. Built specifically to answer this
  task's core question but **not yet actually used** - the plan was to
  compare `AS_Rifle_Reload`'s `hand_r` bone against
  `/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Overlay/M4A1/ALS_Props_M4A1_Aim_Sweep`'s
  `hand_r` (a known-good "holding this exact rifle" reference animation, no
  dedicated Rifle animations exist elsewhere in ALS's own content) at a few
  representative times, to see whether the mismatch is a roughly-constant
  delta (usable directly as the two offset fields) or drifts too much for a
  constant offset to fix at all.
- All of the above is uncommitted in the `ClaudeUnrealMCP` submodule and in
  `ALSHost` itself as of this writing - per the user's system-level
  instructions, only commit when explicitly asked, so this is intentionally
  left staged, not committed.

## Immediate next steps, in order

1. Re-test the debug tuning tool in PIE now that `compile_blueprint` has been
   called; check the log for the widget's diagnostic `UE_LOG` lines to
   confirm whether the fix actually worked, rather than assuming it did.
2. Regardless of (1)'s outcome, run `compare_anim_bone_pose` on
   `AS_Rifle_Reload` vs `ALS_Props_M4A1_Aim_Sweep` (`hand_r`, a couple of
   sampled times each) to get real numbers rather than guessing offsets.
3. Apply whatever offset comes out of (2) via the Details-panel fallback path
   (always works, no dependency on the debug tool) and confirm visually with
   a normal R-key reload in PIE.
4. If the debug tool ends up working, remove the temporary `UE_LOG` calls
   from `ALSRifleReloadTuningWidget.cpp`.
5. Once the offset is confirmed correct, update AGENTS.md's "current status"
   paragraph to reflect the resolved state (it currently describes this as
   unresolved).
