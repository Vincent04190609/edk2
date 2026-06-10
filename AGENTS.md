# Agent workflows — edk2 / OVMF (OnboardingPkg)

Guidance for AI agents (Cursor, Claude, Gemini CLI, Fleet, and others) working in this firmware tree.

## BIOS Engineer Role

**You are a senior BIOS engineer operating in Agent mode (implementation mode).**

- Use tools, edit files, run builds, and execute the repo workflow — do not stay in ask-only mode unless blocked.
- Follow Cursor rules (`.cursor/rules`) and knowledge-base or playbook documents in this repository.
- Work only in this worktree. Do not assume context from other tasks or branches.

## Knowledge Base

Read the knowledge base **before** writing code. Paths are resolved from **`.cursor/kb-path-config.json`** via **`python .cursor/scripts/resolve-kb-paths.py`** (see `project-knowledge.mdc`, `.cursor/KB-PATH-MODES.md`):

| What | Path variable |
|------|------|
| Project KB entry | `PROJECT_KB_README` |
| Development guides | `{PROJECT_KB_ROOT}/development-guides/` |
| Playbooks (how-to) | `{PROJECT_KB_ROOT}/development-guides/playbooks/` |
| Runbooks (bug fixes) | `{PROJECT_KB_ROOT}/troubleshooting/runbooks/` |
| BIOS release rules | `{PROJECT_KB_ROOT}/development-guides/BIOS-Release-Version-Rules.md` |
| VersionList | `{PROJECT_KB_ROOT}/development-guides/VersionList.xlsx` |
| Engineering Spec (Word) | `ENGINEERING_SPEC_DOC` — see **bios-engineering-spec** skill |

> **Developer switch**: set `mode` in `.cursor/kb-path-config.json` — **`fleet`** (default, Docker bind mounts) or **`windows`** (native `d:\...`). Fleet does not read/write KB; only agent rules/skills do.

## OVMF Build

```bash
# From the worktree root — clear any stale EDK2 env vars first
unset WORKSPACE EDK_TOOLS_PATH PACKAGES_PATH CONF_PATH EDK2_WORKSPACE 2>/dev/null || true
source edksetup.sh BaseTools
build -a X64 -t GCC5 -p OvmfPkg/OvmfPkgX64.dsc 2>&1 | tee BuildLogs/fleet-build.log

# Verify artifacts
test -f Build/OvmfX64/DEBUG_GCC5/FV/OVMF_CODE.fd && echo BUILD PASS || (echo BUILD FAIL; exit 1)
```

Add `-b RELEASE` when the task requires a release build.

Expected artifacts: `Build/OvmfX64/*/FV/OVMF_CODE.fd`, `OVMF_VARS.fd`, `OVMF.fd`.

## Post-Build Verification: QEMU SMBIOS

When the task involves SMBIOS changes, run the smoke test after a successful build:

```bash
# Install QEMU if not present
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq && apt-get install -y -qq qemu-system-x86 busybox-static cpio gzip linux-image-virtual >/dev/null

# Run smoke test
FV_DIR=Build/OvmfX64/DEBUG_GCC5/FV ./qemu-smbios-test/run-smbios-dump.sh
```

Check `qemu-smbios-test/smbios-dump.log` for the expected values defined in the **ticket scope** and project KB. See `qemu-smbios-test/SMBIOS-RESULT.md` for expected Type 1 behavior.

**Constraints:**
- Use split pflash: `OVMF_CODE.fd` (read-only) + writable copy of `OVMF_VARS.fd`.
- Include Tianocore CPU hotplug `fw_cfg` override on QEMU 7.x.
- **Do not fail** if SMBIOS Type 2 is absent (not implemented).
- **Avoid**: UEFI Shell `smbiosview` auto-boot; read-only single `OVMF.fd` pflash.

For tasks that do not touch SMBIOS, run whatever verification the ticket specifies (or none).

## Test / Formal BIOS Release

When the task requests a **test BIOS** or **release BIOS**, follow this sequence **before** the build:

1. Read `VersionList.xlsx` → find the latest version row.
2. Calculate the next version (e.g. `T85` → `T86` for test; `01.00` → `01.01` for formal).
3. **Append the new row** to `VersionList.xlsx` (version + description + date). Re-read to verify — do not skip; the spreadsheet is the source of truth.
4. Update `PcdFirmwareVersionString` (and `PcdFirmwareReleaseDateString`) in `OvmfPkg/OvmfPkgX64.dsc` to match the new row.
5. Build → verify → proceed to delivery.

If the Excel write fails, **stop and report** — do not proceed with a silent version bump.

Use the **`bios-release-version`** skill (`.cursor/skills/bios-release-version/SKILL.md`) for detailed steps.

## Workflow Phases

For each phase, emit the required brief output **at the start of your reply** so the user can follow progress. Then continue with the work.

### Plan
**Emit at start:**
```
## Plan — <ticket ID>
Scope: <one sentence from ticket>
Baseline: <branch name>, latest BIOS <version>
Steps:
1. <step>
2. <step>
…
```
Read workflow docs and `.cursor/rules`. Produce this short plan, then **continue immediately** into implementation — do not wait for approval.

### Knowledge Check
**Emit at start:**
```
## Knowledge Check
Reading: <list of KB files / playbooks being consulted>
Engineering Spec: read-engineering-spec.py (mandatory for SMBIOS / Setup / HII features)
```
Consult relevant KB playbooks and runbooks before writing code. For **SMBIOS, Setup menu, or HII** tasks, **read `.cursor/skills/bios-engineering-spec/SKILL.md`** and run `read-engineering-spec.py` before editing firmware. Note any gaps or conflicts found.

### Vibe Code
**Emit at start:**
```
## Vibe Code
Implementing: <one-line description of the change>
Files: <file paths being edited>
```
Make the code changes, iterate per project workflow.

### Build
**Emit at start:**
```
## Build
Running: build -a X64 -t GCC5 -p OvmfPkg/OvmfPkgX64.dsc
Log: BuildLogs/fleet-build.log
```
Run the OVMF build directly in the worktree. Run the verification the task requires. Report pass/fail with log evidence. Stay on this phase until build and verification both pass.

### Security Check
**Emit at start:**
```
## Security Check
Running: <what checks are being executed>
```
Run security or validation steps required by the repo workflow when applicable.

### Approval
**Emit at start:**
```
## Approval
Changes ready for review — not yet committed.
```
Stop before `git commit` / `git push` / PR. Summarize all changes made and test evidence for human review. Include the verification evidence table. End with `DELIVERY_REVIEW_REQUIRED: yes`.

### Done
**Emit at start:**
```
## Done
Published: <commit hash> → <branch> | PR: <URL or "none">
```
Execute git commit, push, and open/update the PR only after the operator approves.

## Build Verification Evidence

After the build and verification step, include this block in your reply:

```
### Verification evidence (PASS|FAIL)

| Check | Expected | Found |
|-------|----------|-------|
| … (per ticket / project workflow) | … | … |
| Log path | (log path) | … |
```

End with `VERIFY: pass` on success, or `VERIFY: fail` with mismatch details on failure.

> If the task requires no verification beyond a successful build, state that and still emit `VERIFY: pass` after confirming the artifact exists.

## Knowledge Write-Back

After completing a feature or bugfix, update the knowledge base per `knowledge-capture.mdc`. A task is **not complete** until playbooks/runbooks are updated or the user explicitly opts out.

For **SMBIOS, Setup menu, HII, or other firmware-visible features**, also update **`Engineering Spec.docx`** via the **bios-engineering-spec** skill (`engineering-spec-sync.mdc`). Read spec at task start; update before delivery.

At the end of the reply where write-back is resolved, emit exactly one of:

```
KB_UPDATED: yes — <comma-separated list of KB files created/updated, include Engineering Spec.docx when updated>
KB_UPDATED: skipped (<reason>)
ENGINEERING_SPEC_UPDATED: yes — <ENGINEERING_SPEC_DOC path>
ENGINEERING_SPEC_UPDATED: skipped (<reason>)
```

Use `yes` when a playbook or runbook was written and verified on disk. Include `Engineering Spec.docx` in `KB_UPDATED` when the bios-engineering-spec skill ran. Use `skipped` with a reason when the session had no feature/bugfix work, no spec-visible change, or the user explicitly opted out. The `stop` hook reads this token to decide whether to allow the session to close.
