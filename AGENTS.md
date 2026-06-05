# Agent workflows — Feature-Jira-523 (edk2 / OVMF)

Guidance for Cursor and Paperclip agents working in this firmware tree.

## BIOS Engineer

**Role**: Build and validate OVMF firmware with OnboardingPkg features.

**Knowledge base** (read first per `lookup-order`):

- Onboarding `PROJECT_KB_ROOT` → [DockerImage README](file:///mnt/d/VibeCoding/Projects/Onboarding/DockerImage/README.md)
- Playbook: [qemu-smbios-smoke-test](file:///mnt/d/VibeCoding/Projects/Onboarding/development-guides/playbooks/qemu-smbios-smoke-test.md)

### Build workflow

1. Validate environment (`docker images uefi-edk2-golden`).
2. Mount repo into golden container; `source edksetup.sh BaseTools`.
3. Build: `build -a X64 -t GCC5 -p OvmfPkg/OvmfPkgX64.dsc` (add `-b RELEASE` if needed).
4. Verify `Build/OvmfX64/*/FV/OVMF_CODE.fd`, `OVMF_VARS.fd`, `OVMF.fd`.

### Post-build validation: QEMU SMBIOS (Type 1)

After a successful build, run the SMBIOS emulation smoke test:

```bash
./qemu-smbios-test/run-smbios-dump.sh
# RELEASE:
FV_DIR=Build/OvmfX64/RELEASE_GCC5/FV ./qemu-smbios-test/run-smbios-dump.sh
```

**Check** `qemu-smbios-test/smbios-dump.log` for:

- `SMBIOS 2.8 present`
- `efi: EFI v2.70 by Vibe-Factory`
- Custom Type 1: `Vibe-Factory`, `OVMF`, `0FEDCBA987654321`

**Do not fail** if SMBIOS Type 2 is missing (not implemented yet).

**Mandatory constraints** (see `.cursor/rules/qemu-smbios-test.mdc`):

- Run QEMU inside Docker golden image (install `qemu-system-x86` in container).
- Use split pflash: `OVMF_CODE.fd` (ro) + writable `OVMF_VARS.fd` copy.
- Include Tianocore CPU hotplug `fw_cfg` override on QEMU 7.x.

**Avoid**: UEFI Shell `smbiosview` auto-boot, host QEMU without Docker, read-only single `OVMF.fd` pflash.

### Agent prompt (short)

> Build OVMF in Docker golden image. After build, run `./qemu-smbios-test/run-smbios-dump.sh` and confirm Type 1 strings in `smbios-dump.log`. See `qemu-smbios-test/SMBIOS-RESULT.md` for expected dual Type 1 behavior.
