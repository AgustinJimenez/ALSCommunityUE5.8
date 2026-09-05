"""Blender --factory-startup -b --python this.py -- <UAL1_Standard.glb> <output.fbx>."""
import bpy,json,pathlib,sys
args=sys.argv[sys.argv.index('--')+1:]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=args[0])
arm=bpy.data.objects['Armature']; action=bpy.data.actions['Punch_Cross']
arm.animation_data_create()
for track in arm.animation_data.nla_tracks: track.mute=True
arm.animation_data.action=action
if action.slots: arm.animation_data.action_slot=action.slots[0]
out={'fps':bpy.context.scene.render.fps,'range':list(action.frame_range),'scale':list(arm.scale),'frames':[]}
for i in range(31):
 bpy.context.scene.frame_set(int(action.frame_range[0]+i*(action.frame_range[1]-action.frame_range[0])/30))
 out['frames'].append({b.name:list((arm.matrix_world@b.matrix).translation) for b in arm.pose.bones})
pathlib.Path(args[1]).with_suffix('.poses.json').write_text(json.dumps(out))
bpy.context.scene.frame_start=int(action.frame_range[0]);bpy.context.scene.frame_end=int(action.frame_range[1])
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
for o in bpy.data.objects:
 if o.type=='MESH' and o.find_armature()==arm:o.select_set(True)
bpy.context.view_layer.objects.active=arm
from mathutils import Matrix
arm.data.transform(Matrix.Scale(100,4))
for obj in bpy.context.selected_objects:
 if obj.type=='MESH':obj.data.transform(Matrix.Scale(100,4))
for layer in action.layers:
 for strip in layer.strips:
  for bag in strip.channelbags:
   for curve in bag.fcurves:
    if curve.data_path.endswith('location'):
     for key in curve.keyframe_points:
      key.co.y*=100;key.handle_left.y*=100;key.handle_right.y*=100
bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=0.01
bpy.ops.export_scene.fbx(filepath=args[1],use_selection=True,add_leaf_bones=False,bake_anim=True,bake_anim_use_all_actions=False,bake_anim_use_nla_strips=False,bake_anim_simplify_factor=0,object_types={'ARMATURE','MESH'})
