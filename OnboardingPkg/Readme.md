# OnboardingPkg

OEM platform features for the Onboarding BIOS project.

## AcpiEcIoDispatch — ports 0x62 / 0x66

Dispatches vendor command **0x59** on port **0x66** with sub-commands on port **0x62**.

### Sub-command 0xD0 (parameter write)

| Step | Port | Value |
|------|------|-------|
| 1 | 0x66 | `0x59` (command) |
| 2 | 0x62 | `0xD0` (sub-command) |
| 3+ | 0x62 | parameter bytes |

Call **`Flush()`** on the protocol when the command packet is complete (unless the buffer fills, which auto-dispatches).

### Sub-command 0xD5 (SMBIOS Type 1 serial read)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | `0xD5` |
| 3+ | 0x62 | IN | serial ASCII bytes until `0x00` |

### Sub-command 0xD6 (SMBIOS Type 1 manufacturer read)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | `0xD6` |
| 3+ | 0x62 | IN | manufacturer ASCII bytes until `0x00` |

### Sub-command 0xD9 (temperature read)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | `0xD9` |
| 3 | 0x62 | IN | `0x32` (50 °C) |

### Integration

1. Build with `OvmfPkg/OvmfPkgX64.dsc` (includes `AcpiEcIoDispatchDxe`).
2. From platform EC access or SMM I/O trap, on each OUT to 0x62/0x66:

```c
ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL  *Dispatch;

gBS->LocateProtocol (&gOnboardingAcpiEcIoDispatchProtocolGuid, NULL, (VOID **)&Dispatch);
Dispatch->ProcessWrite (0x66, CmdByte);
Dispatch->ProcessWrite (0x62, DataByte);
// after last parameter byte (0xD0 path):
Dispatch->Flush ();
```

3. For 0xD5 serial read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD5);
while (EFI_SUCCESS == Dispatch->ProcessRead (0x62, &Byte)) {
  // consume Byte until NUL
}
```

4. For 0xD6 manufacturer read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD6);
while (EFI_SUCCESS == Dispatch->ProcessRead (0x62, &Byte)) {
  // consume Byte until NUL
}
```

5. For 0xD9 temperature read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD9);
Dispatch->ProcessRead (0x62, &Byte);  // Byte == 0x32 (50 C)
```

6. Register a custom handler for 0xD0:

```c
Dispatch->Register59D0Handler (MyHandler59D0);
```

### PCDs

| PCD | Default |
|-----|---------|
| `PcdAcpiEcDataPort` | 0x62 |
| `PcdAcpiEcCmdStatusPort` | 0x66 |
| `PcdAcpiEc59MaxParamBytes` | 64 |

### Note on OVMF / QEMU

QEMU may emulate the ACPI EC at these ports. For OS/tool traffic to reach this dispatch, wire `ProcessWrite` / `ProcessRead` from your EC driver or add an SMM I/O trap on hardware platforms.
