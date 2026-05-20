# Git Commit Guide for SMBIOS Changes (FWB-175)

## Problem Summary
When trying to commit SMBIOS changes, git shows many unrelated files including:
- Modified submodule: `SecurityPkg/DeviceSecurity/SpdmLib/libspdm`
- Build artifacts: `BUILD_COMPLETE.md`, `build-output.log`

## Root Cause
The submodule appears modified due to Windows filesystem limitations:
- Nested submodule `pyca-cryptography` has test files with paths exceeding Windows 260-character limit
- Git cannot checkout these files, causing permanent "modified" status
- This is NOT caused by your changes - it's a Windows + git submodules issue

## Solution: Selective Staging

**DO NOT use `git add -A` or `git add .`** - these will include the problematic submodule.

Instead, commit only your actual SMBIOS changes:

### Step 1: Check what you actually modified
```bash
git diff --name-only
```

### Step 2: Stage ONLY your SMBIOS files
For example, if you modified SMBIOS Type 1:
```bash
git add ArmPkg/Universal/Smbios/SmbiosMiscDxe/Type01/MiscSystemManufacturer.uni
git add OvmfPkg/Include/Dsc/OvmfPkg.dsc.inc
git add OvmfPkg/OvmfPkgX64.dsc
```

### Step 3: Verify staging
```bash
git status
```
Should show ONLY your intended files in "Changes to be committed"

### Step 4: Commit
```bash
git commit -m "FWB-165: Your SMBIOS changes description"
```

## What's Already Fixed

✅ `.gitignore` updated to exclude build artifacts:
- `BUILD_COMPLETE.md`
- `build-output.log`
- `*.log`

These files will no longer appear in git status as untracked.

## Important Notes

1. **Ignore the submodule modification** - Don't try to fix it, don't commit it
2. **Always use selective staging** - Only `git add` the specific files you intentionally changed
3. **The submodule issue is harmless** - It only affects what you see in `git status`, not your actual work
4. **For stable demo codebase** - This approach keeps your commits clean and focused on your actual changes

## Quick Reference Commands

```bash
# See what YOU changed (not submodules)
git diff --name-only

# Stage specific file
git add path/to/your/file.c

# Stage multiple files
git add file1.c file2.h file3.dsc

# Check what's staged
git status

# Commit
git commit -m "FWB-165: Description of changes"

# If you accidentally staged the submodule, unstage it:
git restore --staged SecurityPkg/DeviceSecurity/SpdmLib/libspdm
```

## Result
Your commits will now contain ONLY your actual SMBIOS code changes, without:
- ❌ Submodule updates from online libraries
- ❌ Build artifacts
- ❌ Auto-generated files
- ✅ Clean, focused commits for demo purposes
