/** @file
  ACPI EC ports 0x62 (data) / 0x66 (command) dispatch protocol.

  Command flow (cmd 0x59 on port 0x66):

  Sub-command 0xD0:
    1. Host writes 0x59 to port 0x66
    2. Host writes 0xD0 to port 0x62 (sub-command)
    3. Host writes remaining parameter bytes to port 0x62

  Sub-command 0xD2 (return test BIOS version):
    1. Host writes 0x59 to port 0x66
    2. Host writes 0xD2 to port 0x62 (sub-command)
    3. Host reads ASCII BIOS version bytes from port 0x62 (NUL-terminated)

  Platform EC access code or SMM I/O trap should call ProcessWrite() / ProcessRead()
  for traffic on these ports.

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ONBOARDING_ACPI_EC_IO_DISPATCH_H_
#define ONBOARDING_ACPI_EC_IO_DISPATCH_H_

#include <Uefi/UefiBaseType.h>

#define ONBOARDING_ACPI_EC_CMD_VENDOR_59   0x59
#define ONBOARDING_ACPI_EC_SUBCMD_59_D0   0xD0
#define ONBOARDING_ACPI_EC_SUBCMD_59_D2   0xD2

/**
  Handler for command 0x59 / sub-command 0xD0.

  @param[in]  ParamData   Bytes received on port 0x62 after the sub-command byte.
  @param[in]  ParamSize   Number of parameter bytes.
**/
typedef
EFI_STATUS
(EFIAPI *ONBOARDING_ACPI_EC_59_D0_HANDLER)(
  IN CONST UINT8  *ParamData,
  IN UINTN        ParamSize
  );

typedef struct _ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL {
  EFI_STATUS
  (EFIAPI *PROCESS_WRITE)(
    IN UINT16  Port,
    IN UINT8   Value
    );
  EFI_STATUS
  (EFIAPI *PROCESS_READ)(
    IN  UINT16  Port,
    OUT UINT8   *Value
    );
  EFI_STATUS
  (EFIAPI *REGISTER_59_D0_HANDLER)(
    IN ONBOARDING_ACPI_EC_59_D0_HANDLER  Handler
    );
  EFI_STATUS
  (EFIAPI *FLUSH)(
    VOID
    );
} ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL;

extern EFI_GUID  gOnboardingAcpiEcIoDispatchProtocolGuid;

#endif // ONBOARDING_ACPI_EC_IO_DISPATCH_H_
