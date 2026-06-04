# OnboardingPkg

OEM platform features for the Onboarding BIOS project.

## AcpiEcIoDispatch — ports 0x62 / 0x66

Dispatches vendor command **0x59** on port **0x66** with sub-commands on port **0x62**.

| Sub-command | Direction | Data |
|-------------|-----------|------|
| `0xD0` | IN (params) | Parameter bytes; call `Flush()` when complete |
| `0xD4` | OUT (reads) | SMBIOS Type 1 product name (ASCII until `0x00`) |
| `0xD5` | OUT (reads) | SMBIOS Type 1 serial number (ASCII until `0x00`) |
| `0xD6` | OUT (reads) | SMBIOS Type 1 manufacturer (ASCII until `0x00`) |
| other `0xDx` | OUT (reads) | `0x0000` (two `0x00` bytes, unsupported response) |

### Sub-command 0xD0 (parameter write)

| Step | Port | Value |
|------|------|-------|
| 1 | 0x66 | `0x59` (command) |
| 2 | 0x62 | `0xD0` (sub-command) |
| 3+ | 0x62 | parameter bytes |

Call **`Flush()`** on the protocol when the command packet is complete (unless the buffer fills, which auto-dispatches).

### Sub-command 0xD4 (SMBIOS Type 1 product name read)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | `0xD4` |
| 3+ | 0x62 | IN | product name ASCII bytes until `0x00` |

### Sub-command 0xD5 (SMBIOS Type 1 serial read)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | `0xD5` |
| 3+ | 0x62 | IN | serial ASCII bytes until `0x00` |

### Unsupported sub-command 0xDx (e.g. 0xD8)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | unsupported `0xDx` (not `0xD0`, `0xD4`, `0xD5`, `0xD6`) |
| 3–4 | 0x62 | IN | `0x00`, `0x00` (16-bit unsupported response `0x0000`) |

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

### Sub-command 0xDA (fan speed read)

| Step | Port | Direction | Value |
|------|------|-----------|-------|
| 1 | 0x66 | OUT | `0x59` |
| 2 | 0x62 | OUT | `0xDA` |
| 3 | 0x62 | IN | `0xF0` (full speed) |

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

3. For 0xD4 product name read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD4);
while (EFI_SUCCESS == Dispatch->ProcessRead (0x62, &Byte)) {
  // consume Byte until NUL
}
```

4. For 0xD5 serial read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD5);
while (EFI_SUCCESS == Dispatch->ProcessRead (0x62, &Byte)) {
  // consume Byte until NUL
}
```

5. For 0xD6 manufacturer read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD6);
while (EFI_SUCCESS == Dispatch->ProcessRead (0x62, &Byte)) {
  // consume Byte until NUL
}
```

6. For 0xD9 temperature read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD9);
Dispatch->ProcessRead (0x62, &Byte);  // Byte == 0x32 (50 C)
```

7. For 0xDA fan speed read:

```c
Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xDA);
Dispatch->ProcessRead (0x62, &Byte);  // Byte == 0xF0 (full speed)
```

8. For unsupported 0xDx (example `0xD8`):

```c
UINT8  Byte;
UINTN  Index;

Dispatch->ProcessWrite (0x66, 0x59);
Dispatch->ProcessWrite (0x62, 0xD8);
for (Index = 0; Index < 2; Index++) {
  Dispatch->ProcessRead (0x62, &Byte);  // expect 0x00 each
}
```

9. Register a custom handler for 0xD0:

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
