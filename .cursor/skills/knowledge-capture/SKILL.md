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

## Quick Decision

| User task | Document type | Target folder |
|-----------|---------------|---------------|
| New feature, enhancement, config change | **Playbook** | `development-guides/playbooks/` |
| Bug fix, boot hang, regression | **Runbook** | `troubleshooting/runbooks/` |
| Release / test or formal version | **Version log** | `development-guides/VersionList.xlsx` (+ `CHANGELOG.md` if used) |

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

1. Read `development-guides/BIOS-Release-Version-Rules.md`.
2. Update `development-guides/VersionList.xlsx` (use the **xlsx** skill to read/write).
3. Optionally add a line to `development-guides/CHANGELOG.md`.

### Step 5 — Hand off to user

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
- `development-guides/BIOS-Release-Version-Rules.md` — versioning
- `DockerImage/README.md` — build environment
