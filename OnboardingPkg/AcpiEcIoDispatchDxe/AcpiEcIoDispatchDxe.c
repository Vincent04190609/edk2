/** @file
  DXE driver — installs ACPI EC 0x62/0x66 dispatch protocol.

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/OnboardingAcpiEcIoDispatchLib.h>
#include <Protocol/OnboardingAcpiEcIoDispatch.h>
#include <Protocol/Smbios.h>

STATIC ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL  mAcpiEcIoDispatch = {
  OnboardingAcpiEcIoDispatchLibProcessWrite,
  OnboardingAcpiEcIoDispatchLibProcessRead,
  OnboardingAcpiEcIoDispatchLibRegister59D0,
  OnboardingAcpiEcIoDispatchLibFlush,
};

STATIC EFI_HANDLE  mHandle = NULL;

STATIC
CHAR8 *
GetSmbiosStringByIndex (
  IN CHAR8  *OptionalStrStart,
  IN UINT8  Index
  )
{
  UINTN  StrSize;

  if ((OptionalStrStart == NULL) || (Index == 0)) {
    return NULL;
  }

  do {
    Index--;
    if (Index == 0) {
      return OptionalStrStart;
    }

    StrSize = AsciiStrSize (OptionalStrStart);
    OptionalStrStart += StrSize;
  } while (*OptionalStrStart != 0);

  return NULL;
}

STATIC
EFI_STATUS
LoadSmbiosType1Fields (
  VOID
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TYPE           SmbiosType;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  SMBIOS_TABLE_TYPE1        *Type1;
  CHAR8                     *Strings;
  CHAR8                     *Serial;
  CHAR8                     *Manufacturer;

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "AcpiEcIoDispatchDxe: Smbios protocol not found %r\n", Status));
    OnboardingAcpiEcIoDispatchLibSet59D5Serial ("");
    OnboardingAcpiEcIoDispatchLibSet59D6Manufacturer ("");
    return Status;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  SmbiosType   = EFI_SMBIOS_TYPE_SYSTEM_INFORMATION;
  Status       = Smbios->GetNext (
                           Smbios,
                           &SmbiosHandle,
                           &SmbiosType,
                           &Record,
                           NULL
                           );
  if (EFI_ERROR (Status) || (Record == NULL)) {
    DEBUG ((DEBUG_WARN, "AcpiEcIoDispatchDxe: Type 1 SMBIOS record not found %r\n", Status));
    OnboardingAcpiEcIoDispatchLibSet59D5Serial ("");
    OnboardingAcpiEcIoDispatchLibSet59D6Manufacturer ("");
    return EFI_NOT_FOUND;
  }

  Type1        = (SMBIOS_TABLE_TYPE1 *)Record;
  Strings      = (CHAR8 *)Record + Record->Length;
  Serial       = GetSmbiosStringByIndex (Strings, Type1->SerialNumber);
  Manufacturer = GetSmbiosStringByIndex (Strings, Type1->Manufacturer);

  {
    CHAR8  *ProductName;
    ProductName = GetSmbiosStringByIndex (Strings, Type1->ProductName);
    if ((ProductName == NULL) || (ProductName[0] == '\0')) {
      DEBUG ((DEBUG_WARN, "AcpiEcIoDispatchDxe: Type 1 product name string missing\n"));
      OnboardingAcpiEcIoDispatchLibSet59D4ProductName ("");
    } else {
      OnboardingAcpiEcIoDispatchLibSet59D4ProductName (ProductName);
    }
  }

  if ((Serial == NULL) || (Serial[0] == '\0')) {
    DEBUG ((DEBUG_WARN, "AcpiEcIoDispatchDxe: Type 1 serial string missing\n"));
    OnboardingAcpiEcIoDispatchLibSet59D5Serial ("");
  } else {
    OnboardingAcpiEcIoDispatchLibSet59D5Serial (Serial);
  }

  if ((Manufacturer == NULL) || (Manufacturer[0] == '\0')) {
    DEBUG ((DEBUG_WARN, "AcpiEcIoDispatchDxe: Type 1 manufacturer string missing\n"));
    OnboardingAcpiEcIoDispatchLibSet59D6Manufacturer ("");
  } else {
    OnboardingAcpiEcIoDispatchLibSet59D6Manufacturer (Manufacturer);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AcpiEcIoDispatchDxeEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  OnboardingAcpiEcIoDispatchLibInit ();
  (VOID)LoadSmbiosType1Fields ();

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
    "AcpiEcIoDispatchDxe: cmd 0x59 / sub 0xD0, 0xD4, 0xD5, 0xD6 dispatch ready (ports 0x%02x data, 0x%02x cmd)\n",
    PcdGet16 (PcdAcpiEcDataPort),
    PcdGet16 (PcdAcpiEcCmdStatusPort)
    ));
  return EFI_SUCCESS;
}
