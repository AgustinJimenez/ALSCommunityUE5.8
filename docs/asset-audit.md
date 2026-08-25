# Asset Audit

Snapshot taken 2026-08-25 against `Plugins/ALS-Community-UE5` content, using
the Asset Registry's real referencer graph (`get_referencers`), not just file
size or folder scanning.

## Largest tracked files

Actual git-tracked files (excludes build artifacts and the `ClaudeUnrealMCP`
submodule):

| Size | Asset |
|---|---|
| 10.5 MB | `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/ALS_AnimBP.uasset` |
| 9.97 MB | `Resources/Readme_Content_2.gif` (plugin README asset, not gameplay content) |
| 6.6 MB | `Content/AdvancedLocomotionV4/Environment/Materials/Textures/T_Tiles_N.uasset` |
| 5.8 MB | `Content/AdvancedLocomotionV4/Environment/Materials/Textures/T_Tiles_M.uasset` |
| 5.5 MB | `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/Materials/Textures/UE4_Mannequin__normals.uasset` |
| 3.7 MB | `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/Meshes/Mannequin.uasset` |

Total plugin content is roughly 552 MB — normal for a UE character/animation
asset pack, nothing anomalous.

## Unused-asset scan

Queried `AssetRegistryHelpers.get_asset_registry().get_referencers(...)` for
every asset under `/ALSV4_CPP` (337 total). 26 came back with zero
referencers.

### False positives (expected to show 0 referencers)

These are legitimately used but referenced outside the normal asset
dependency graph:

- `Content/AdvancedLocomotionV4/Levels/ALS_DemoLevel` — entry-point map, referenced via project config (`DefaultEngine.ini`), not by other assets
- `Content/AdvancedLocomotionV4/Levels/ALS_GridLevel` — same; alternate demo map, openable directly
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/Editor` (AnimBlueprint) — Persona editor-internal preview object
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/ALS_Mannequin_T_Pose` (AnimSequence) — skeleton reference pose used by the Persona editor, not by other content

### Genuine cleanup candidates

No referencers found, and no obvious editor/config reason for that:

**Orphaned footstep sounds** (6):
- `Content/AdvancedLocomotionV4/Audio/Footsteps/Concrete_Pivot_03`
- `Content/AdvancedLocomotionV4/Audio/Footsteps/Concrete_Pivot_04`
- `Content/AdvancedLocomotionV4/Audio/Footsteps/Concrete_Run_01`
- `Content/AdvancedLocomotionV4/Audio/Footsteps/Concrete_Run_02`
- `Content/AdvancedLocomotionV4/Audio/Footsteps/Concrete_Run_03`
- `Content/AdvancedLocomotionV4/Audio/Footsteps/Concrete_Run_04`

**Orphaned locomotion animations** (3):
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_CRF_WalkPose`
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_CRF_Walk_B`
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Base/Locomotion/ALS_CRF_Walk_F`

**Orphaned M4A1 overlay animations** (3):
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Overlay/M4A1/ALS_Props_M4A1_Run_F`
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Overlay/M4A1/ALS_Props_M4A1_Sprint_F`
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/AnimationExamples/Overlay/M4A1/ALS_Props_M4A1_Sprint_F_Impulse`

**Cut/unused character features**:
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/Materials/M_Cape` (Material — looks like a cut cape feature)
- `Content/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/Meshes/Proxy` (SkeletalMesh)

**Orphaned curves**:
- `Content/AdvancedLocomotionV4/Data/Curves/AnimationBlendCurves/StrideBlend_N_Run_V` (CurveVector)
- `Content/AdvancedLocomotionV4/Data/Curves/AnimationBlendCurves/StrideBlend_N_Walk_V` (CurveVector)
- `Content/AdvancedLocomotionV4/Data/Curves/CameraBlendCurves/CameraLerp_3` (CurveFloat)

**Orphaned grid-level decoration set** (only relevant to the also-orphaned `ALS_GridLevel`, consistent with that map no longer placing them):
- `Content/AdvancedLocomotionV4/Environment/Materials/M_GridLevel_Objects` (Material)
- `Content/AdvancedLocomotionV4/Environment/Materials/Textures/T_Checker_Noise_M` (Texture2D)
- `Content/AdvancedLocomotionV4/Environment/Meshes/Complex_Fountain_02` (StaticMesh)
- `Content/AdvancedLocomotionV4/Environment/Meshes/Simple_Platform_2x2` (StaticMesh)
- `Content/AdvancedLocomotionV4/Environment/Meshes/Simple_TitleBoard` (StaticMesh)

## Re-running this audit

```python
import unreal
ar = unreal.AssetRegistryHelpers.get_asset_registry()
all_assets = ar.get_assets_by_path('/ALSV4_CPP', recursive=True)
unused = []
for a in all_assets:
    pkg = a.package_name
    refs = ar.get_referencers(pkg, unreal.AssetRegistryDependencyOptions())
    if len(refs) == 0:
        unused.append((str(pkg), str(a.asset_class_path.asset_name)))
for pkg, cls in unused:
    print('UNUSED', cls, pkg)
```

Run via the in-editor Python console (or `execute_console_command` with a
`py` prefix through the ClaudeUnrealMCP bridge), then cross-check any new
results against the false-positive list above before deleting anything.
