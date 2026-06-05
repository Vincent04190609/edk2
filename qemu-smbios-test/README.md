# QEMU SMBIOS smoke test

Post-build validation: boot built OVMF in QEMU and dump SMBIOS Type 1 (and check Type 2 presence).

## Quick start

From the edk2 repo root, after a successful OVMF build:

```bash
./qemu-smbios-test/run-smbios-dump.sh
```

RELEASE build:

```bash
FV_DIR=Build/OvmfX64/RELEASE_GCC5/FV ./qemu-smbios-test/run-smbios-dump.sh
```

## Outputs

| File | Description |
|------|-------------|
| `smbios-dump.log` | Full serial boot log |
| `SMBIOS-RESULT.md` | Expected results and interpretation |

## Requirements

- Docker with `uefi-edk2-golden:latest`
- Built firmware: `Build/OvmfX64/*/FV/OVMF_CODE.fd` and `OVMF_VARS.fd`

QEMU is installed inside the container at run time (`qemu-system-x86`); it is not bundled in the golden image.

## Agent / rule references

- `.cursor/rules/qemu-smbios-test.mdc` — Cursor rule for agents
- `AGENTS.md` — BIOS Engineer workflow
- KB playbook: Onboarding `development-guides/playbooks/qemu-smbios-smoke-test.md`

## Pass criteria (summary)

- `SMBIOS 2.8 present` in log
- `efi: EFI v2.70 by Vibe-Factory`
- Custom Type 1 strings: `Vibe-Factory`, `OVMF`, `0FEDCBA987654321`
- Type 2: absent unless added in `SmbiosPlatformDxe.c` (expected today)
