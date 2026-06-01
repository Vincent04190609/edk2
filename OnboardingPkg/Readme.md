# OnboardingPkg

OEM platform features for the Onboarding BIOS project.

## AcpiEcIoDispatch — ports 0x62 / 0x66

Dispatches vendor command **0x59** on port **0x66** with sub-commands on port **0x62**.

### Sequence — sub-command 0xD0 (parameters)

| Step | Port | Value |
|------|------|-------|
| 1 | 0x66 | `0x59` (command) |
| 2 | 0x62 | `0xD0` (sub-command) |
| 3+ | 0x62 | parameter bytes |

Call **`Flush()`** on the protocol when the command packet is complete (unless the buffer fills, which auto-dispatches).

### Sequence — sub-command 0xD1 (return BIOS version)

| Step | Port | Value / action |
|------|------|----------------|
| 1 | 0x66 | `0x59` (command) |
| 2 | 0x62 | `0xD1` (sub-command) |
| 3+ | 0x62 | **IN** read — ASCII BIOS version from `PcdFirmwareVersionString` (null-terminated) |

Default handler sources the version string from **`PcdFirmwareVersionString`** (same as SMBIOS / release tag).

### Integration

1. Build with `OvmfPkg/OvmfPkgX64.dsc` (includes `AcpiEcIoDispatchDxe`).
2. From platform EC access or SMM I/O trap, on each OUT to 0x62/0x66:

```c
ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL  *Dispatch;

gBS->LocateProtocol (&gOnboardingAcpiEcIoDispatchProtocolGuid, NULL, (VOID **)&Dispatch);
Dispatch->ProcessWrite (0x66, CmdByte);
Dispatch->ProcessWrite (0x62, DataByte);
// after last parameter byte (0xD0 path only):
Dispatch->Flush ();
```

3. After **0x59 / 0xD1**, read version bytes from port **0x62**:

```c
UINT8  Byte;
while (!EFI_ERROR (Dispatch->ProcessRead (0x62, &Byte))) {
  // consume Byte (ends at null terminator)
}
```

4. Register custom handlers:

```c
Dispatch->Register59D0Handler (MyHandler59D0);
Dispatch->Register59D1Handler (MyHandler59D1);
```

### PCDs

| PCD | Default |
|-----|---------|
| `PcdAcpiEcDataPort` | 0x62 |
| `PcdAcpiEcCmdStatusPort` | 0x66 |
| `PcdAcpiEc59MaxParamBytes` | 64 |

### Note on OVMF / QEMU

QEMU may emulate the ACPI EC at these ports. For OS/tool traffic to reach this dispatch, wire `ProcessWrite` / `ProcessRead` from your EC driver or add an SMM I/O trap on hardware platforms.
