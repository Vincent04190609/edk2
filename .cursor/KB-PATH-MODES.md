# Knowledge Base Path Modes

Agents (Cursor, Claude, Gemini, Fleet SDK, etc.) resolve KB paths from **one config file** in this repo. Fleet does **not** read or write KB files — only agent rules/skills do.

## Switch (developer setting)

Edit **`.cursor/kb-path-config.json`** and set `mode`:

| Mode | When to use | Runtime paths |
|------|-------------|---------------|
| **`fleet`** (default) | Agent runs inside a Linux Docker container (Fleet worktree) with host bind mounts | `/home/workspace/project_kb/{ProjectName}/`, `/home/workspace/common_kb/` |
| **`windows`** | Agent runs on native Windows (Cursor IDE, no WSL/Docker) | `d:\VibeCoding\Projects\{ProjectName}\`, `d:\VibeCoding\Common\` |

Also set `project_name` when switching products (e.g. `Onboarding`).

### Example — Fleet / Docker (default)

```json
{
  "mode": "fleet",
  "project_name": "Onboarding",
  "paths": {
    "fleet": {
      "project_kb_projects_root": "/home/workspace/project_kb",
      "common_kb_root": "/home/workspace/common_kb"
    },
    ...
  }
}
```

### Example — Windows native

```json
{
  "mode": "windows",
  "project_name": "Onboarding",
  ...
}
```

## Resolve paths (all agents)

Run from the edk2 repo root:

```bash
python .cursor/scripts/resolve-kb-paths.py
python .cursor/scripts/resolve-kb-paths.py --key PROJECT_KB_ROOT
python .cursor/scripts/resolve-kb-paths.py --format shell
```

Output includes `PROJECT_KB_ROOT`, `COMMON_KB_ROOT`, `PROJECT_KB_README`, `ENGINEERING_SPEC_DOC`, `CODE_REPO_ROOT`, and flags for out-of-workspace writes.

**Rules reference**: `.cursor/rules/project-knowledge.mdc` (always read config + run resolver before KB I/O).

## Fleet bind mounts (mode: `fleet`)

The container must bind-mount host KB folders. See `Fleet/docker-compose.yml`:

- `AIFW_HOST_KB_PROJECTS_ROOT` → `/home/workspace/project_kb`
- `AIFW_HOST_COMMON_KB_ROOT` → `/home/workspace/common_kb`

Fleet only provides mounts; Cursor rules/skills perform all KB reads and write-backs.

## Windows mode on WSL (edge case)

If `mode` is `windows` but the agent shell is Linux/WSL, the resolver auto-translates `d:\...` → `/mnt/d/...` when the Windows path is not directly visible. Prefer **`fleet`** mode inside Docker; use **`windows`** on native Windows Cursor.

## Write-back

KB roots are **outside** the edk2 git workspace in both modes. Sandboxed writes may fail — use elevated permissions and **read the file back** to confirm (see `knowledge-capture.mdc`).

## Changing project

1. Set `project_name` in `.cursor/kb-path-config.json`.
2. Ensure the matching folder exists under the mounted/host Projects tree.
3. Update `PROJECT.md` in that KB repo if needed.

Do not hardcode `Onboarding` or `d:\VibeCoding\...` in rules/skills — use the config + resolver.
