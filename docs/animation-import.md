# Universal Animation Library → ALS

This workflow repaired UAL1's `Punch_Cross`. Use it as the starting point for
other clips, but validate each result; UAL2 and the remaining animations have
not been verified through this pipeline.

## 1. Export a consistent source

Use `Unreal-Godot/UAL1_Standard.glb` from the downloaded Universal Animation
Library pack. Prefer this source over the old Unity FBX used in earlier attempts.

Run the reusable [Blender export script](../AGENT_TASKS/punch_repair/export_punch_cm.py):

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.0/blender.exe' --factory-startup -b --python AGENT_TASKS/punch_repair/export_punch_cm.py -- '<absolute path to UAL1_Standard.glb>' '<absolute output.fbx>'
```

The script currently selects `Punch_Cross`; change that action name for another
clip. It exports only that action and its bound mesh, without NLA strips, other
actions, or extra leaf bones. It also writes sampled source positions to a
`.poses.json` sidecar for comparison.

The essential unit conversion is **consistent across rest pose and animation**:
scale armature rest data, mesh vertices, and action location keys by 100, then
set Blender scene units to `0.01`. Do not scale only the mesh or only the bones.
The verified export is retained at
`SourceArt/Animations/UAL1_Punch_Cross_CM.fbx`.

## 2. Import into a fresh source asset

- Run `Interchange.FeatureFlags.Import.FBX 0` in the current editor session;
  this avoids the known Interchange/MCP TaskGraph crash path.
- Import **skeletal mesh plus animation**, at uniform scale **1**. Disable
  material, texture, and physics-asset creation for this animation workflow.
- Use a new asset name for experiments. In-place replacement can silently
  retain old data. Do not import onto the ALS skeleton yet.
- Explicitly save all generated packages, including the animation and skeleton,
  and verify their files exist. Check editor connectivity after import.

Before retargeting, compare reference and animated root scales and sample the
pelvis, hands, and feet. The broken export had reference root scale **100** but
animated scale **1**: a ~92cm reference pelvis became ~0.825cm in animation.
Fix that mismatch at export; do not compensate with large retargeter offsets.
The corrected source punch begins with its pelvis at ~82.51cm.

## 3. Retarget onto ALS

The reusable assets live under `/Game/ALSHost/Animations/PunchRepair/`:

| Asset | Purpose |
|---|---|
| `PunchSourceCM`, `PunchSourceCM_Skeleton`, `PunchSourceCM_Anim` | Verified imported source |
| `IKRig_PunchSource`, `IKRig_PunchTarget` | Source and ALS chain definitions |
| `IKRT_PunchClean` | Working retargeter |

Use `IKRetargetBatchOperation.run_batch_retarget` with the source animation,
source mesh, ALS `Mannequin` target mesh, and this retargeter. Write a separate
candidate animation first. Verify preview-mesh references actually resolve.

For the verified GLB source, both pelvis scale settings are **1**. The retarget
poses were reset, then source chains auto-aligned to ALS's A-pose. There is no
manual root-yaw or arm-rotation correction; the IK Rig operation remains enabled.
Do not reset the working calibration every run or reuse the old Unity-FBX
offsets. Duplicate the retargeter before experimenting with another rig.

## 4. Validate, then replace

Compare the original and candidate across the full timeline. Check start,
strike, recovery, and end from multiple angles; a single good-looking frame
can hide a badly twisted chain. Inspect actual mesh deformation at elbows,
wrists, and fingers as well as bone positions. Account for the mesh's rotation
relative to the actor when judging forward direction.

For Blender inspection, export the UE animation with
`FbxExportOption.export_preview_mesh = True`. Combining an animation-only
export with a separately imported mesh introduced misleading scale problems
in this investigation.

Back up the current asset before replacing it. Renaming the old asset moves
existing references with it, so explicitly repoint consumers to the replacement.
Stop PIE before compiling affected Blueprints; save and read back assignments.
**Never reimport the source FBX directly into the final ALS-targeted animation.**

The current punch is `/Game/ALSHost/Animations/AS_Punch_Cross`, assigned to
`ALS_CharacterBP → MeleeComponent → FistSwingAnimation`. Its playback test is
`ALSHost.Interaction.ALSInteractionRuntimeTests.MeleeComponent_Attack_PlaysSwingMontage`.
That test checks montage playback, not pose quality; visual checks remain required.

See [the repair record](../AGENT_TASKS/0005_punch_clean_retarget.md) for the
specific findings, backup location, and validation results.
