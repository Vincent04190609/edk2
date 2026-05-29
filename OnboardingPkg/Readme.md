# OnboardingPkg

OEM platform features for the Onboarding BIOS project.

## AcpiEcIoDispatch — ports 0x62 / 0x66

Dispatches vendor command **0x59** on port **0x66** with sub-commands on port **0x62**.

### Sub-command 0xD0 (parameters)

| Step | Port | Value |
|------|------|-------|
| 1 | 0x66 | `0x59` (command) |
| 2 | 0x62 | `0xD0` (sub-command) |
| 3+ | 0x62 | parameter bytes |

Call **`Flush()`** when the command packet is complete (unless the buffer fills, which auto-dispatches).

### Sub-command 0xD2 (test BIOS version)

| Step | Port | Value |
|------|------|-------|
| 1 | 0x66 | `0x59` (command) |
| 2 | 0x62 | `0xD2` (sub-command) |
| 3+ | 0x62 | **read** ASCII BIOS version (`PcdFirmwareVersionString`, NUL-terminated) |

After step 2, each **read** from port **0x62** returns the next version character until the NUL byte is returned.

### Integration

1. Build with `OvmfPkg/OvmfPkgX64.dsc` (includes `AcpiEcIoDispatchDxe`).
2. From platform EC access or SMM I/O trap, on each OUT/IN to 0x62/0x66:

```c
ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL  *Dispatch;

gBS->LocateProtocol (&gOnboardingAcpiEcIoDispatchProtocolGuid, NULL, (VOID **)&Dispatch);
Dispatch->ProcessWrite (0x66, CmdByte);
Dispatch->ProcessWrite (0x62, DataByte);
Dispatch->ProcessRead (0x62, &ReadByte);
// after last parameter byte (0xD0 path):
Dispatch->Flush ();
```

3. Register a custom handler for 0xD0:

```c
Dispatch->Register59D0Handler (MyHandler59D0);
```

### PCDs

| PCD | Default |
|-----|---------|
| `PcdAcpiEcDataPort` | 0x62 |
| `PcdAcpiEcCmdStatusPort` | 0x66 |
| `PcdAcpiEc59MaxParamBytes` | 64 |
| `PcdFirmwareVersionString` (MdeModulePkg) | platform DSC (e.g. `L"T72"`) — source for 0xD2 reads |

### Note on OVMF / QEMU

QEMU may emulate the ACPI EC at these ports. For OS/tool traffic to reach this dispatch, wire `ProcessWrite` / `ProcessRead` from your EC driver or add an SMM I/O trap on hardware platforms.
