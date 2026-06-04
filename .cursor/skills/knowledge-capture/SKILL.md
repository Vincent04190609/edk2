---
name: knowledge-capture
description: >-
  Update the Primary Knowledge Repository after implementing BIOS features or fixing bugs.
  Use when completing feature work (SMBIOS, setup menu, drivers, PCDs), bugfixes (boot hang,
  build failures), or when the user asks to document, capture, or update the knowledge base,
  playbook, or runbook. Also use when the user says "task done" and knowledge write-back is required.
---

# Knowledge Capture (Write-Back to 2nd Brain)

Captures reusable FW engineering knowledge into the Primary Knowledge Repository so future agent sessions and engineers benefit.

**Paths**: Resolve from **`project-knowledge.mdc`**:

- `PROJECT_KB_ROOT` — where to write playbooks/runbooks
- `PROJECT_KB_README` — `{PROJECT_KB_ROOT}\README.md` (derive; read first)
- `COMMON_KB_ROOT` / `COMMON_FROM_PROJECT` — shared standards only

**Read discipline**: Follow `lookup-order` before implementing — check for existing playbooks/runbooks first.

> **⚠ WSL / out-of-workspace write**: `PROJECT_KB_ROOT` is **outside the agent workspace** at a `/mnt/<drive>/...` path (translate `d:\...` → `/mnt/d/...`). The default sandbox blocks writes there, so `Write`/`StrReplace`/sandboxed shell writes return **`Permission denied`** and the file silently never lands. Perform KB writes with **elevated permissions**, then **read the file back from disk to confirm** before reporting success. See "Runtime path resolution" in `project-knowledge.mdc`.

## Quick Decision

| User task | Document type | Target folder |
|-----------|---------------|---------------|
| New feature, enhancement, config change | **Playbook** | `development-guides/playbooks/` |
| Bug fix, boot hang, regression | **Runbook** | `troubleshooting/runbooks/` |
| Release / test or formal version | **Version log** | Run **bios-release-version** skill first → `development-guides/VersionList.xlsx` (+ `CHANGELOG.md` if used) |

## Workflow

### Step 1 — Classify and search

1. Determine: **feature (playbook)** vs **bug (runbook)** vs **release (version)**.
2. List `playbooks/` or `runbooks/` for an existing doc on the same topic.
3. If a doc exists → **update** it (add section, refresh steps). If not → **create** from template.

### Step 2 — Draft the document

Copy structure from:

- Features: `development-guides/playbooks/TEMPLATE-feature-playbook.md`
- Bugs: `troubleshooting/runbooks/TEMPLATE-bug-runbook.md`

Use **kebab-case** filenames (e.g. `smbios-add-serial-number.md`, `power-on-hang-at-pei.md`).

### Step 3 — Update indexes

1. Add a bullet link in `playbooks/README.md` or `runbooks/README.md`.
2. If it is a major guide, also link from `development-guides/README.md` or `troubleshooting/README.md`.

### Step 4 — Version tracking (if applicable)

If the work produced a new **test** (Txx) or **formal** (xx.xx) BIOS version:

1. Run the **bios-release-version** skill (`.cursor/skills/bios-release-version/SKILL.md`) — this handles Excel read/increment/write, verification, and `.dsc` update.
2. Optionally add a line to `development-guides/CHANGELOG.md` after the release skill completes.

Do **not** guess the next version or skip Excel — delegate entirely to **bios-release-version**.

### Step 5 — Hand off to user

**Before replying**, `ls`/read each created or updated KB file back from its `/mnt/<drive>/...` path to confirm it landed on disk. If a write was blocked, redo it with elevated permissions. Do not list a file as "Created/Updated" unless verified.

Reply with:

```markdown
## Knowledge base updated

| Action | Path |
|--------|------|
| Created/Updated | `{PROJECT_KB_ROOT}/...` |

**Summary**: [1-2 sentences]

Please review before committing the knowledge base repo.
```

If KB files live outside the edk2 git repo, remind the user to commit/push the `PROJECT_KB_ROOT` repository separately.

## Playbook content checklist (features)

- [ ] Title and one-line goal
- [ ] Platform / package (e.g. OvmfPkg X64)
- [ ] Prerequisites (tools, Docker image `uefi-edk2-golden:latest`)
- [ ] Files and modules changed
- [ ] PCDs, tokens, HII, SMBIOS type (if applicable)
- [ ] Build commands
- [ ] Validation / acceptance criteria
- [ ] Pitfalls and related docs
- [ ] **ACPI EC sub-command added or changed** → create or update `development-guides/playbooks/acpi-ec-dispatch-59-dXX.md` and update `playbooks/README.md` index; verify the sub-command constant is in `OnboardingPkg/Include/Protocol/OnboardingAcpiEcIoDispatch.h` and `IsSupported59SubCmd()` in the dispatch library

## Runbook content checklist (bugs)

- [ ] Symptom and how to reproduce
- [ ] Environment (platform, BIOS version, toolchain)
- [ ] Root cause
- [ ] Fix (files, commits, issue ID)
- [ ] Verification steps
- [ ] Prevention / follow-ups

## User prompt templates

Share these with the user for consistent requests:

**Feature:**

```text
TYPE: Feature
GOAL: [one sentence]
PLATFORM: [e.g. OvmfPkg X64]
ACCEPTANCE: [how to verify]
KB: create playbook [name] | update existing: [path]
```

**Bugfix:**

```text
TYPE: Bugfix
SYMPTOM: [what fails]
PLATFORM: [board / OVMF]
ACCEPTANCE: [how to verify fix]
KB: create runbook [name] | update existing: [path]
```

**Close the loop:**

```text
Task done. Update project knowledge base (playbook or runbook) and show paths for review.
```

## Related rules and docs

- `.cursor/rules/knowledge-capture.mdc` — enforcement
- `.cursor/rules/lookup-order.mdc` — read order
- `.cursor/skills/bios-release-version/SKILL.md` — Test/Formal version bump workflow
- `development-guides/BIOS-Release-Version-Rules.md` — versioning rules (referenced by bios-release-version skill)
- `DockerImage/README.md` — build environment
