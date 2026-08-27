# 0002 - ALS's Q-held debug overlay menu doesn't respond to mouse wheel

## Symptom

Holding **Q** shows ALS's built-in debug overlay-cycle menu (list of props/
overlay states), but scrolling the mouse wheel does not move the selector.
User reports this worked before this session's changes.

## Investigation and conclusion

Initially suspected the new mouse-wheel zoom feature (`CameraZoomInputAction`,
added this session to `UALSWeaponFireComponent`) was stealing the
`MouseWheelAxis` input out from under ALS's own `DebugOverlayMenuCycleAction`
(both bound to the same key, in different `UInputMappingContext`s -
`IMC_Zoom`/`IMC_Fire` vs ALS's own `DebugInputMappingContext`).

Two fixes were tried:
1. Checking `PC->IsInputKeyDown(EKeys::Q)` in our zoom handler to skip
   applying zoom while Q is held.
2. Moving zoom into its own `IMC_Zoom` mapping context added at priority -1,
   below ALS's own `DefaultInputMappingContext` (1) and
   `DebugInputMappingContext` (0) - per Epic's own doc comment on
   `AddMappingContext`, higher-priority contexts are evaluated first and, if
   they consume input, block lower-priority ones.

Neither fixed it. **Isolation test**: with the zoom feature's C++ binding and
mapping-context-add both fully disabled (no zoom mapping active in PIE at
all), Q+scroll *still* didn't move the selector. This conclusively rules out
zoom (or anything else in `UALSWeaponFireComponent`) as the cause - the
problem exists independent of anything built this session.

Zoom has been restored (re-enabled after the isolation test confirmed it
wasn't at fault).

## Status: RESOLVED - root-caused as genuinely broken vendored content, replaced

Follow-up investigation (reading `DebugComponent`'s full EventGraph via
`read_event_graph_detailed`) found the real cause: `OverlayStateButtons`, the
array `OverlayMenuCycle`'s connected logic depends on to move a selector, is
never populated anywhere in the Blueprint - no code path, including
`Construct`, ever creates or adds to it. This is not a regression from
anything built this session and not environmental; it is incomplete vendored
content in ALS-Community-UE5 itself. There was nothing to fix at the input
layer because the feature was never finished upstream.

Fix: built a full native-C++ replacement instead of patching the broken one -
`UALSOverlayStateOptionWidget` (one clickable/hoverable row) and
`UALSDebugPropMenuWidget` (enumerates every `EALSOverlayState` value and
spawns one row per value), reusing the same Q-held `DebugOverlayMenuInputAction`.
Two more real bugs surfaced getting *that* working (a CanvasPanel-rooted
widget reporting zero size when embedded in a VerticalBox, and
`DebugComponent`'s own menu-close logic reverting the overlay state back to
Default via a `SelectOverlayState` call on its own broken switcher widget) -
both are written up in detail in `AGENTS.md` under "On building a replacement
debug prop-picker menu". Confirmed working live by the user: holding Q shows
the new menu, hover highlights rows, clicking changes the character's overlay
state immediately, and it now survives releasing Q.
