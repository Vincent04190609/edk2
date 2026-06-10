# QEMU SMBIOS dump result (OVMF DEBUG_GCC5)

Firmware under test:
- `Build/OvmfX64/DEBUG_GCC5/FV/OVMF_CODE.fd` + `OVMF_VARS.fd`
- `PcdFirmwareVendor` = `Vibe-Factory`
- `PcdFirmwareVersionString` = `T83`

## Verdict

| Item | Emulation OK? | Notes |
|------|---------------|-------|
| SMBIOS Type 1 | **Yes (with caveat)** | Two Type 1 records are published |
| SMBIOS Type 2 | **No** | Not present in the SMBIOS table set |
| OS-visible DMI | **Yes** | Linux sees `SMBIOS 2.8 present` and firmware vendor `Vibe-Factory` |

## SMBIOS Type 1 (System Information)

### Record A — from QEMU `fw_cfg` (installed first)

| Field | Value |
|-------|-------|
| Manufacturer | `QEMU` |
| Product Name | `Standard PC (Q35 + ICH9, 2009)` |
| Version | `pc-q35-7.1` |
| Serial Number | *(empty)* |

### Record B — added by `OvmfPkg/SmbiosPlatformDxe/SmbiosPlatformDxe.c`

| Field | Value |
|-------|-------|
| Manufacturer | `Vibe-Factory` |
| Product Name | `OVMF` |
| Version | `1.0` |
| Serial Number | `123456789` |

Firmware debug log during boot also confirms:

```
FirmwareVendor:            "Vibe-Factory"
FirmwareVersionString:     "T83"
Adding SMBIOS Type 1 with SerialNumber: 123456789
```

Linux kernel DMI line (uses first Type 1 + BIOS Type 0):

```
DMI: QEMU Standard PC (Q35 + ICH9, 2009), BIOS T83 2026-06-04 22:25:39
efi: EFI v2.70 by Vibe-Factory
```

## SMBIOS Type 2 (Base Board Information)

**Not published.** Raw table types observed under QEMU:

- Type 0 (BIOS) — `Vibe-Factory` / `T83`
- Type 1 (System) — QEMU default + custom `Vibe-Factory` record
- Type 3 (Chassis)
- Type 4 (Processor)
- Type 16, 17, 19, 32 (memory/address)

No Type 2 structure was added by QEMU defaults or by current OVMF platform code.

## Reproduce

```bash
./qemu-smbios-test/run-smbios-dump.sh
```

Full serial log: `qemu-smbios-test/smbios-dump.log`

## Implications for OnboardingPkg EC reads (0x59 / 0xD4–0xD6)

`AcpiEcIoDispatchDxe` loads Type 1 strings via `EFI_SMBIOS_PROTOCOL`. With two Type 1 handles present, the DXE loader uses whichever record `GetNext()` returns first — verify that path if EC read-back must always return the `Vibe-Factory` / `123456789` record.

To emulate Type 2 in QEMU, add a Type 2 table in `SmbiosPlatformDxe.c` (similar to the existing custom Type 1 block).
