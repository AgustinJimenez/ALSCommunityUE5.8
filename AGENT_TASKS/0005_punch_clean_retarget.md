# Punch animation repair — 2026-09-05

Reusable workflow: [docs/animation-import.md](../docs/animation-import.md).
This file records the specific repair and its validation.

## Result

Replaced `/Game/ALSHost/Animations/AS_Punch_Cross` with a fresh retarget from
UAL1's **Unreal-Godot/UAL1_Standard.glb**, action `Punch_Cross` (1 second).
`ALS_CharacterBP.MeleeComponent.FistSwingAnimation` explicitly repointed,
compiled without warnings/errors, and saved. Previous animation preserved as
`/Game/ALSHost/Animations/PunchRepair/AS_Punch_Cross_BeforeCleanImport`.

## Findings

- The live old retargeter did **not** match 0004's claimed identity baseline:
  source clavicle_l had yaw 84°, upperarm_l yaw 34°, lowerarm_l yaw -180°,
  plus root yaw 180°. Its source preview mesh also referenced a missing asset.
- Plain Blender FBX export produced a source reference root scale of 100 but
  animated root scale of 1: reference pelvis height 91.67cm, animated 0.825cm.
  Retargeting this explained the old scale_vertical=100 workaround.
- Clean export uses centimeters end to end: scale armature rest data, mesh
  vertices, and selected action location keys by 100; scene units 0.01;
  export just the selected punch action. UE imports at scale 1. Source pelvis
  now starts at 82.51cm and target at 87.08cm with both pelvis scale settings 1.
- This GLB needs **no root yaw correction**. Reset both retarget poses, then
  auto-align source chains to the target A-pose. Do not copy the old Unity-FBX
  root/arm offsets. Skeletal mesh component space and actor facing differ;
  the old viewer's universal '+X forward' label was not a reliable reference.

## Reusable assets

Under `/Game/ALSHost/Animations/PunchRepair/`:
`PunchSourceCM`, `PunchSourceCM_Skeleton`, `PunchSourceCM_Anim`,
`IKRig_PunchSource`, `IKRig_PunchTarget`, `IKRT_PunchClean`.
These are isolated copies; the old shared retargeter is unchanged.
Superseded non-CM source and `_Clean1` experiments were removed after checking references.

## Validation

Sampled 31 poses across the full second; visually compared source, old and new
at 0, 0.3, 0.6, 1.0 seconds from multiple angles. New left hand stays in guard
and right arm extends/retracts like the source; old left arm dropped/twisted.
Exported candidate **with preview mesh** and rendered the actual ALS mannequin
in Blender to verify hand and elbow deformation at strike/recovery.
Animation-only FBX plus separately imported mesh is an unreliable visual check
(object animation scaling can double-convert units); use export_preview_mesh.

The referenced Blueprint compiled cleanly and its saved CDO reads the correct
animation path and `Grounded Slot`. Targeted montage CQTest **passed**, exit 0:
`ALSHost.Interaction.ALSInteractionRuntimeTests.MeleeComponent_Attack_PlaysSwingMontage`.
Log: `Saved/Logs/PunchRepairTest.log`. The test loads the saved asset from disk;
it checks montage playback, not pose quality (covered by the comparisons above).

Working inspection scripts/data/renders are in `Saved/punch_*`. Durable source:
`SourceArt/Animations/UAL1_Punch_Cross_CM.fbx`. Recreate it with Blender:
`--factory-startup -b --python AGENT_TASKS/punch_repair/export_punch_cm.py -- <UAL1_Standard.glb> <output.fbx>`.
Import skeletal mesh plus animation with legacy FBX (`Interchange.FeatureFlags.Import.FBX 0`),
scale 1, no materials/textures/physics asset. Retarget `PunchSourceCM_Anim`
through `IKRT_PunchClean` using `IKRetargetBatchOperation.run_batch_retarget`.
Do not directly reimport the source FBX into the ALS-targeted final animation.
