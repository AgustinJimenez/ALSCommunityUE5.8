# MCP Notes: Quirks and Limitations

Working notes from live sessions driving this project through `ClaudeUnrealMCP`
and Epic's native `ModelContextProtocol`. See `README.md` for setup; this file
is about what actually happens once you're connected.

## Current status

Only `ClaudeUnrealMCP` (TCP `9877`) has ever been confirmed listening in this
project, despite **Auto Start Server** appearing enabled in *Editor
Preferences → Model Context Protocol*. Epic's native server (HTTP `8000`)
has not come up on its own; if you need it, run
`ModelContextProtocol.StartServer 8000` in the in-editor console and verify
with `Get-NetTCPConnection -LocalPort 8000 -State Listen` before relying on
it.

## AnimGraph state machines are mostly invisible to normal tooling

A Blueprint's `FunctionGraphs`/`UbergraphPages` (what most Blueprint tooling
walks) do **not** include the graphs nested inside an AnimGraph state
machine — one graph per state, one per transition rule. Those are only
reachable by walking `Node->GetSubGraphs()` recursively starting from the
top-level graphs (see `Blueprint->GetAllGraphs()` + subgraph walk in
`MCPServerHelpers.cpp` and `MCPServerNodeEdit.cpp`).

- `read_function_graphs` does **not** descend into these — it only sees the
  top-level AnimGraph, not the state/transition sub-graphs.
- `reconstruct_node` already did the full recursive walk and could find a
  node anywhere; `delete_node` didn't until we fixed it (see
  `asset-audit.md`'s sibling commit history / the `ClaudeUnrealMCP` repo's
  commit `cbfadf5`).
- Python's `get_editor_property('nodes')` on an `EdGraph` is blocked
  ("protected and cannot be read"), and `NodeGuid` is similarly blocked on
  individual nodes. The reliable fallback when Python reflection refuses is
  the in-editor console: `obj dump "<full object path>"` prints both as
  plain text. Full object paths for nested graphs look like:

  ```
  /Game/.../ALS_AnimBP.ALS_AnimBP:AimOffsetBehaviors.AnimGraphNode_StateMachine_0.Aim Offset Behavior States.AnimStateNode_1.Look Towards Camera.AnimGraphNode_StateMachine_0.Look Towards Camera States.AnimStateTransitionNode_75.Transition
  ```

  i.e. a dot-separated walk down through each nested state machine / state /
  transition, ending at the actual sub-graph or node name. Object paths
  containing spaces need to be double-quoted for the console parser.

- `AnimationBlueprintLibrary.GetNodesOfClass` (backing `bp.get_nodes_of_class()`
  in Python) only accepts `AnimGraphNode_Base` subclasses — it will not find
  a `K2Node_CallFunction` or `K2Node_AnimGetter` even though those are
  perfectly normal contents of a transition rule graph. Use the manual
  `GetAllGraphs()` + subgraph walk instead for anything that isn't a native
  anim node.

## Removing nodes safely

Never remove a Blueprint graph node via the graph's raw `RemoveNode()`. It
only drops the node from the `Nodes` array — it does not break pin links or
notify owning Blueprint extensions. For a node inside an AnimGraph state
machine sub-graph, this leaves `AnimBlueprintExtension_StateMachine`'s cached
`TargetRootNode` dangling, which crashes the editor with an internal engine
assertion (`AnimBlueprintExtension_StateMachine.cpp:528`) on the *next*
compile — not immediately, which makes it a nasty one to trace back.
Always use `FBlueprintEditorUtils::RemoveNode()` instead. (Fixed in
`ClaudeUnrealMCP` commit `cbfadf5`; if you're on an older submodule pin,
update first.)

## Known gaps — things this MCP setup currently can't do

- **No generic "add arbitrary node" tool.** The typed `add_*` tools
  (`add_component`, `add_material_expression`, `add_widget`,
  `add_montage_section`, `add_set_struct_node`, etc.) cover specific node/
  asset categories, but there's no way to spawn an arbitrary `K2Node`
  subclass (a Comment node, a `Branch`, a `Print String`, an `AnimGetter`
  like "Current State Time") by class name. If a node is fully *missing*
  (not just corrupted), you currently have to add it by hand in the editor.
- **No PIE input injection.** Nothing in this MCP setup can inject simulated
  keyboard/mouse input into a running Play-In-Editor session. `play_in_editor`
  can start/stop PIE, but testing actual gameplay input (movement, aiming,
  weapon actions if this project grows that direction) still needs a human
  at the keyboard, or Windows-level UI automation as a fallback (see
  ResidentHorrorV1's `docs/unreal-mcp.md` for that pattern).
- **No packaging/cooking tool.** Building a shippable package isn't exposed;
  that still means `RunUAT.bat BuildCookRun` by hand.
- **No dedicated lighting/navmesh build tool.** Use `execute_console_command`
  with the relevant console command (e.g. `BuildLighting`) — it works, but
  output isn't captured in the tool response, only in the log file.
- **`execute_console_command` never returns output.** Every console command
  (including `py ...` scripts) requires a follow-up `read_log` (or direct
  log-file read) to see what actually happened. Easy to forget mid-session.
- **Editor crashes are silent to the caller.** If a command crashes the
  editor process, the MCP client just gets a generic connection-reset error
  — nothing distinguishes "editor crashed" from "network hiccup." Always
  check `Get-Process -Name UnrealEditor` after any suspicious error before
  assuming a transient failure.

## Roadmap: Mover / Motion Matching / deeper GAS support

If this project (or a successor, e.g. a GASP-ALS-R-based one) moves onto
Epic's newer stack, `ClaudeUnrealMCP` will need real work to keep up.
Checked the source directly (not assumed) as of commit `cbfadf5`:

- **GAS**: `create_gameplay_ability` / `create_gameplay_effect` exist but
  are thin — they just spawn a Blueprint parented to `GameplayAbility`/
  `GameplayEffect`. No dedicated tooling for ability-graph editing,
  attribute sets, or gameplay-effect specs beyond the generic
  `manage_gameplay_tags`.
- **Mover**: zero support. No mention anywhere in the plugin source, not
  even scaffolding.
- **Motion Matching / Pose Search**: zero support. Same.

This isn't a hard blocker — Mover movement modes and Pose Search nodes are
still ordinary Blueprint/AnimGraph constructs under the hood, so the
generic tools (`read_blueprint`, `add_component`, `set_component_property`,
and the now-fixed `delete_node`/`reconstruct_node` that reach nested
graphs) should still work to some degree. But expect the same category of
blind spot we just fixed for AnimGraph state machines to resurface —
Pose Search nodes live inside AnimGraph too, and nobody's verified that
path works yet. Building proper support is the same shape of work as the
`delete_node` fix in this doc: find the gap live, trace the exact engine
API via `obj dump`/source reading, add a typed C++ handler, test, commit
to the `ClaudeUnrealMCP` repo, bump the submodule pointer.

## Re-running the asset audit

See `asset-audit.md` for the unused-asset scan and its re-run snippet.
