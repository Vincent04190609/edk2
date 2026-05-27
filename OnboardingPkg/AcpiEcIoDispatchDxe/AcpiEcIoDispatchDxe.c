/** @file
  DXE driver — installs ACPI EC 0x62/0x66 dispatch protocol.

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/OnboardingAcpiEcIoDispatchLib.h>
#include <Protocol/OnboardingAcpiEcIoDispatch.h>

STATIC ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL  mAcpiEcIoDispatch = {
  OnboardingAcpiEcIoDispatchLibProcessWrite,
  OnboardingAcpiEcIoDispatchLibRegister59D0,
  OnboardingAcpiEcIoDispatchLibFlush,
};

STATIC EFI_HANDLE  mHandle = NULL;

EFI_STATUS
EFIAPI
AcpiEcIoDispatchDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  OnboardingAcpiEcIoDispatchLibInit ();

  Status = gBS->InstallProtocolInterface (
                  &mHandle,
                  &gOnboardingAcpiEcIoDispatchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mAcpiEcIoDispatch
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "AcpiEcIoDispatchDxe: InstallProtocol failed %r\n", Status));
    return Status;
  }

  DEBUG ((
    DEBUG_INFO,
    "AcpiEcIoDispatchDxe: cmd 0x59 / sub 0xD0 dispatch ready (ports 0x%02x data, 0x%02x cmd)\n",
    PcdGet16 (PcdAcpiEcDataPort),
    PcdGet16 (PcdAcpiEcCmdStatusPort)
    ));
  return EFI_SUCCESS;
}
