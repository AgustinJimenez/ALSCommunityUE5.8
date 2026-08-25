# Architecture: C++ vs Blueprint

Answers a question that comes up naturally given the plugin's name
(`ALSV4_CPP`) and the fact that the one real bug we've hit so far
(`asset-audit.md`, the `Current State Time` node) was in a Blueprint.

## What's actually C++

`Plugins/ALS-Community-UE5/Source/ALSV4_CPP/` — ~8,100 lines across 22
files:

- `ALSBaseCharacter` / `ALSCharacter` — the core character class
- `ALSCharacterMovementComponent` — movement state (grounded/airborne, gait,
  rotation mode, stance)
- `ALSPlayerController`, `ALSPlayerCameraManager` — input and camera
- `ALSCharacterAnimInstance` — the animation **data layer**: exposes
  grounded/airborne state, speed, gait, lean amount, aim offset values, etc.
  as C++ properties/functions
- `ALSMantleComponent`, `ALSDebugComponent` — mantling and debug overlay
- `ALSMathLibrary` — math helpers
- AI controller + behavior-tree tasks
- Several `AnimNotify`/`AnimNotifyState` classes

## What's Blueprint, and why that's correct

**`ALS_AnimBP`** (the AnimGraph) is a Blueprint whose parent class is the
C++ `ALSCharacterAnimInstance` — confirmed via `read_blueprint`. This is not
incomplete conversion: Unreal's **AnimGraph** (state machines, blend spaces,
the visual blending graph) is structurally an editor/Blueprint construct.
There is no supported way to author an AnimGraph in pure C++, even in a
plugin built almost entirely in C++ like this one. The C++ base class
supplies the *data*; the Blueprint AnimGraph does the *blending*.

The remaining 16 Blueprint classes under `Content/AdvancedLocomotionV4/`
all fall into categories that are expected to stay Blueprint regardless of
how C++-heavy a project is:

| Blueprint | Parent | Vars / Components | Role |
|---|---|---|---|
| `ALS_CharacterBP` | `ALSCharacter` (C++) | 12 / 2 | thin subclass, wires mesh/asset refs |
| `ALS_AIBP` | `ALSCharacter` (C++) | 12 / 1 | same, AI variant |
| `ALS_Player_Controller` | `ALSPlayerController` (C++) | 0 / 0 | empty shell, holds class-default overrides only |
| `ALS_GameMode_SP` | `GameModeBase` (engine) | 0 / 1 | wires `DefaultPawnClass`/etc. |
| `DebugComponent`, `MantleComponent`, `ALS_Controller_AI`, `ALS_PlayerCameraManager` | matching C++ classes | — | thin config subclasses |
| `Calculate_RotationAmount`, `Create_LayeringCurves`, `Copy_Curves`, `Create_Curves` | `AnimationModifier` | — | editor-time asset-baking scripts, not runtime code |
| `Sprint_CameraShake` | `CameraShakeBase` | — | data-only curve config |
| `ALS_HUD_Macro_Library` | — | — | trivial UI helper macros |
| `SimpleObjectBuilder`, `SimpleMovingObject` | — | — | demo-content decoration actors |

**Conclusion**: there is no remaining Blueprint logic that represents
unfinished C++ conversion. Heavy logic lives in C++; Blueprint is used only
where Unreal requires it (AnimGraph) or where thin editor-configurable
subclasses are the normal, correct pattern.
