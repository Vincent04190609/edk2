---
name: bios-release-version
description: >-
  Bump Test (Txx) or Formal (xx.xx) BIOS version from VersionList.xlsx, write the new row
  (version, description, date), then update PcdFirmwareVersionString in the platform .dsc.
  Use when implementation is done and the user wants a test BIOS, formal BIOS, release,
  version bump, or says task done with a new BIOS build. Mandatory before marking a BIOS
  release complete. Always read Excel first; never guess the next version.
---

# BIOS Release Version Workflow

Procedural gate for Test and Formal BIOS releases. **Excel is the source of truth** — read it, increment, write it back, verify, then update firmware code.

**Paths**: Resolve from **`project-knowledge.mdc`**:

| Variable | Resolved as |
|----------|-------------|
| `PROJECT_KB_ROOT` | Project knowledge base root |
| `VERSION_RULES` | `{PROJECT_KB_ROOT}/development-guides/BIOS-Release-Version-Rules.md` |
| `VERSION_LIST` | `{PROJECT_KB_ROOT}/development-guides/VersionList.xlsx` |
| `VERSION_FALLBACK` | `{PROJECT_KB_ROOT}/development-guides/config/version_tracking.json` |

**WSL path resolution (MANDATORY)**: `PROJECT_KB_ROOT` is stored as a Windows path (e.g. `d:\VibeCoding\Projects\Onboarding`). When running in WSL/Linux, convert to POSIX before any file operation:

```
d:\VibeCoding\Projects\Onboarding  →  /mnt/d/VibeCoding/Projects/Onboarding
```

Always use the `/mnt/d/...` form when calling Python (pandas/openpyxl) or shell tools from WSL. Never pass `d:\...` to a Linux process.

**Related skills**: Use the **xlsx** skill (`.cursor/skills/xlsx/SKILL.md`) for all Excel read/write operations.

## When to Use

Run this skill when:

- Implementation is complete and a **Test** or **Formal** BIOS release is expected
- User says: "test BIOS", "formal BIOS", "release", "version bump", "T01", "00.01", or similar
- User says "task done" and the work includes a new BIOS build

If release type (Test vs Formal) is unclear, **ask the user** before proceeding.

## Mandatory Order (do not reorder)

```
Read Excel → Increment → Write Excel → Verify Excel → Update .dsc → Report
```

**STOP** if Excel read or write fails. Do not update code or build until Excel is updated and verified.

> ⛔ **Hard stop rule**: If the xlsx skill cannot open, read, or write `{VERSION_LIST}`, reply to the user immediately:
> `"⛔ Excel update FAILED — path: {VERSION_LIST} (WSL: /mnt/d/...). NOT proceeding to .dsc update. Fix the path or file and retry."`
> Do NOT continue to Step 3 or beyond until Excel is confirmed readable and writable.

## Step 1 — Read rules and classify release type

1. Read `{VERSION_RULES}` for naming rules, fallback logic, and backup procedure.
2. Confirm with user (if not already stated):
   - **Test BIOS** → format `Txx` (e.g. T01, T02)
   - **Formal BIOS** → format `XX.XX` (e.g. 00.01, 01.00)

If the user provides a version that already matches the naming rule, use that version instead of auto-incrementing.

## Step 2 — Read last version from Excel

Use the **xlsx** skill to read `{VERSION_LIST}`.

**Schema** (preserve existing template):

| Column | Header | Content |
|--------|--------|---------|
| 1 | Version No | e.g. `T72` or `00.01` |
| 2 | Description | Brief summary of changes |
| 3 | Date | Release date and time (`YYYY-MM-DD hh:mm:ss`) |

**Extraction logic**:

1. Find the **latest row** for the release type being issued:
   - Test: last row whose Version No matches `T` + digits (e.g. `T72`)
   - Formal: last row whose Version No matches `XX.XX` pattern (e.g. `00.01`, `01.05`)
2. Record this as **Last Version**.

**Fallback** (only if Excel is missing, unreadable, or has no matching rows):

1. Read `{VERSION_FALLBACK}`
2. Use `last_test_version` or `last_formal_version` as Last Version
3. **MANDATORY**: Notify the user immediately:
   `"⚠ Excel NOT updated — path unreachable or unreadable. Used fallback JSON ({VERSION_FALLBACK}). Excel MUST be updated manually before this release is considered recorded."`
4. Continue only if user explicitly acknowledges the warning and accepts fallback

## Step 3 — Calculate next version

### Test BIOS

- Format: `Txx` (T01, T02, T03…)
- Increment: add 1 to the numeric suffix
- Example: T72 → T73, T05 → T06
- First build of a new project stage: start at T01

### Formal BIOS

- Format: `XX.XX` (e.g. 00.01, 01.00, 02.00)
- **Minor release** (bug fix / config tweak): increment last two digits → 01.05 → 01.06
- **Major release** (significant feature / new hardware stage): increment first segment, reset minor → 01.05 → 02.00
- If major vs minor is unclear, **ask the user**

Record **Next Version**.

## Step 4 — Write new row to Excel (MANDATORY before code changes)

Use the **xlsx** skill to append a new row to `{VERSION_LIST}`:

| Version No | Description | Date |
|------------|-------------|------|
| Next Version | Brief summary of BIOS changes (from implementation or user) | Current date and time (`YYYY-MM-DD hh:mm:ss`) |

**Rules**:

- Preserve existing Excel format, fonts, and column layout — match the template exactly
- Description should capture key changes (feature, bugfix, issue ID if provided)
- Date must use 24-hour format: `YYYY-MM-DD hh:mm:ss` (e.g. `2026-06-03 14:30:00`)
- Do **not** skip this step even if the user only asked for a code version bump

## Step 5 — Verify Excel update

Re-read `{VERSION_LIST}` and confirm:

- [ ] New row exists as the **latest entry** for that release type
- [ ] Version No matches **Next Version**
- [ ] Description and Date are populated (Date in `YYYY-MM-DD hh:mm:ss` format)

If verification fails:
> ⛔ Reply: `"⛔ Excel verification FAILED — new row not found or data mismatch in {VERSION_LIST}. NOT proceeding to Step 6. Fix Excel and re-verify."`
> Do NOT update `.dsc` until verification passes.

## Step 6 — Update firmware version in code

Update `PcdFirmwareVersionString` in the platform `.dsc` file **after** Excel is verified.

**Default platform** (OVMF X64):

```
OvmfPkg/OvmfPkgX64.dsc
```

Find the line:

```
gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"..."
```

Set the string literal to **Next Version** (e.g. `L"T73"` or `L"00.02"`).

For other platforms, search the repo for `PcdFirmwareVersionString` in the relevant `.dsc`.

## Step 7 — Optional follow-ups

After Excel and `.dsc` are updated:

- Add a line to `{PROJECT_KB_ROOT}/development-guides/CHANGELOG.md` if the project uses it
- Run **knowledge-capture** skill for playbook/runbook write-back (separate from this skill)
- Per `{VERSION_RULES}`: binary backup, `release_history.csv`, `version_tracking.json` sync if applicable

## Handoff template

Reply with:

```markdown
## BIOS release version updated

| Field | Value |
|-------|-------|
| Release type | Test / Formal |
| Last version | T72 |
| Next version | T73 |
| Excel | `{VERSION_LIST}` |
| Firmware PCD | `OvmfPkg/OvmfPkgX64.dsc` |

**Description recorded**: [brief summary]

Please review Excel and .dsc changes before build/commit.
```

If KB files live outside the edk2 git repo, remind the user to commit the `PROJECT_KB_ROOT` repository separately.

## User prompt templates

**Test BIOS release:**

```text
TYPE: Test BIOS release
DESCRIPTION: [brief summary of changes]
PLATFORM: [e.g. OvmfPkg X64]
```

**Formal BIOS release:**

```text
TYPE: Formal BIOS release
DESCRIPTION: [brief summary of changes]
MAJOR: yes | no  (optional — minor bump if omitted)
PLATFORM: [e.g. OvmfPkg X64]
```

**After implementation:**

```text
Implementation done. Bump test BIOS version, update VersionList.xlsx and OvmfPkgX64.dsc.
```

## Checklist (agent self-verify)

- [ ] Read `{VERSION_RULES}` and `{VERSION_LIST}` before incrementing
- [ ] Used xlsx skill for Excel operations
- [ ] Excel updated **before** `.dsc` change
- [ ] Re-read Excel to verify new row
- [ ] `PcdFirmwareVersionString` matches Next Version
- [ ] Reported Last → Next version and file paths to user

## Related docs

- `{VERSION_RULES}` — full naming rules, backup, fallback JSON schema
- `.cursor/skills/xlsx/SKILL.md` — Excel read/write procedures
- `.cursor/skills/knowledge-capture/SKILL.md` — playbook/runbook write-back (run after release if needed)
- `.cursor/rules/knowledge-capture.mdc` — KB write-back enforcement
