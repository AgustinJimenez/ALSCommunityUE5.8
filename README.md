# ALSHost

Minimal C++ host project (Unreal Engine 5.8) for running and testing the
[ALS-Community-UE5](https://github.com/ProjectBorealis/ALS-Community-UE5)
locomotion plugin, with live editor automation via a dual-MCP setup.

## Structure

- `Plugins/ALS-Community-UE5/` — the locomotion plugin (vendored, not a submodule)
- `Plugins/ClaudeUnrealMCP/` — **git submodule** pointing at
  [AgustinJimenez/ClaudeUnrealMCP](https://github.com/AgustinJimenez/ClaudeUnrealMCP).
  Improvements to this plugin should be committed/pushed from inside
  `Plugins/ClaudeUnrealMCP` to its own repo, then the submodule pointer bumped
  here (`git submodule update --remote` or a manual `git add Plugins/ClaudeUnrealMCP`
  after checking out the desired commit).
- `Source/ALSHost/` — empty primary game module, exists only so the project has
  a C++ target to build

## First-time setup (after cloning)

```powershell
git submodule update --init --recursive
cd Plugins/ClaudeUnrealMCP/MCPServer
npm install
```

Then generate project files and build `ALSHostEditor` (Development, Win64) before
opening `ALSHost.uproject`.

## MCP setup

Two MCP layers are wired into this project, mirroring the pattern used in
`ResidentHorrorV1`:

1. **Epic's native `ModelContextProtocol` + `EditorToolset`** — auto-starts an
   HTTP server on `127.0.0.1:8000/mcp` if **Auto Start Server** is enabled in
   *Edit → Editor Preferences → Model Context Protocol* (not on by default —
   enable it once per machine, or run `ModelContextProtocol.StartServer 8000`
   in the in-editor console).
2. **`ClaudeUnrealMCP`** — a TCP server on port `9877`, bridged to Claude Code
   via a Node.js stdio server (`Plugins/ClaudeUnrealMCP/MCPServer/index.js`).
   Registered in `~/.claude.json` under `mcpServers` as `unreal-engine-als`.
   Requires the editor to be running; restart Claude Code after first adding
   the server entry.

## Known quirks

- **Rebuild after a fresh submodule checkout / removed `Binaries`+`Intermediate`**:
  `ClaudeUnrealMCP` ships no prebuilt binaries in its own repo (unlike the copy
  vendored into some other projects). If `Plugins/ClaudeUnrealMCP/Binaries` and
  `Intermediate` are missing (fresh clone, or after `git submodule update`
  resets the submodule), the project won't load the plugin until it's rebuilt:
  regenerate project files and build `ALSHostEditor` before launching the editor.
- **exFAT filesystem**: this repo lives on an exFAT-formatted drive, which does
  not support NTFS junctions/symlinks. Plugins are vendored as real files
  (or a git submodule) rather than linked in from elsewhere.
