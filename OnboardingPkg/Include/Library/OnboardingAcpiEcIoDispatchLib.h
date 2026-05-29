/** @file
  Library for ACPI EC port 0x62 / 0x66 command dispatch state machine.

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef ONBOARDING_ACPI_EC_IO_DISPATCH_LIB_H_
#define ONBOARDING_ACPI_EC_IO_DISPATCH_LIB_H_

#include <Protocol/OnboardingAcpiEcIoDispatch.h>

/**
  Initialize dispatch library state. Call once from DXE driver entry point.
**/
VOID
EFIAPI
OnboardingAcpiEcIoDispatchLibInit (
  VOID
  );

/**
  Register handler for command 0x59 / sub-command 0xD0.
**/
EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibRegister59D0 (
  IN ONBOARDING_ACPI_EC_59_D0_HANDLER  Handler
  );

/**
  Process write to port 0x62 or 0x66.

  @param[in]  Port   I/O port (typically 0x62 or 0x66).
  @param[in]  Value  Byte written.
**/
EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibProcessWrite (
  IN UINT16  Port,
  IN UINT8   Value
  );

/**
  Process read from port 0x62 (e.g. BIOS version after cmd 0x59 / sub 0xD2).

  @param[in]   Port   I/O port (typically 0x62).
  @param[out]  Value  Byte returned to host.
**/
EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibProcessRead (
  IN  UINT16  Port,
  OUT UINT8   *Value
  );

/**
  Complete cmd 0x59 / 0xD0 and invoke the registered handler with collected params.
**/
EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibFlush (
  VOID
  );

/**
  Built-in stub handler for 0x59 / 0xD0 (debug log). Replace via Register59D0().
**/
EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibDefault59D0Handler (
  IN CONST UINT8  *ParamData,
  IN UINTN        ParamSize
  );

#endif // ONBOARDING_ACPI_EC_IO_DISPATCH_LIB_H_
