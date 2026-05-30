/** @file
  DXE driver — installs ACPI EC 0x62/0x66 dispatch protocol.

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/OnboardingAcpiEcIoDispatchLib.h>
#include <Protocol/OnboardingAcpiEcIoDispatch.h>
#include <Protocol/Smbios.h>

STATIC ONBOARDING_ACPI_EC_IO_DISPATCH_PROTOCOL  mAcpiEcIoDispatch = {
  OnboardingAcpiEcIoDispatchLibProcessWrite,
  OnboardingAcpiEcIoDispatchLibProcessRead,
  OnboardingAcpiEcIoDispatchLibRegister59D0,
  OnboardingAcpiEcIoDispatchLibRegister59D3,
  OnboardingAcpiEcIoDispatchLibFlush,
};

STATIC EFI_HANDLE  mHandle = NULL;

STATIC
CHAR8 *
GetSmbiosString (
  IN EFI_SMBIOS_TABLE_HEADER  *Record,
  IN SMBIOS_TABLE_STRING      StringNumber
  )
{
  CHAR8  *String;
  UINTN  Index;

  if ((Record == NULL) || (StringNumber == 0)) {
    return NULL;
  }

  String = (CHAR8 *)Record + Record->Length;
  for (Index = 1; Index < StringNumber; Index++) {
    while (*String != 0) {
      String++;
    }

    String++;
    if (*String == 0) {
      return NULL;
    }
  }

  return String;
}

STATIC
EFI_STATUS
EFIAPI
AcpiEc59D3SmbiosSerialHandler (
  OUT CHAR8  *Response,
  IN  UINTN   ResponseMax,
  OUT UINTN  *ResponseLen
  )
{
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER   *Record;
  SMBIOS_TABLE_TYPE1        *Type1;
  CHAR8                     *Serial;
  UINTN                     SerialLen;
  EFI_STATUS                Status;

  if ((Response == NULL) || (ResponseMax == 0) || (ResponseLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Response[0]  = 0;
  *ResponseLen = 1;

  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "AcpiEcDispatch 59/D3: SMBIOS protocol not found %r\n", Status));
    return EFI_SUCCESS;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  while (TRUE) {
    Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "AcpiEcDispatch 59/D3: SMBIOS Type 1 not found %r\n", Status));
      return EFI_SUCCESS;
    }

    if (Record->Type != EFI_SMBIOS_TYPE_SYSTEM_INFORMATION) {
      continue;
    }

    Type1  = (SMBIOS_TABLE_TYPE1 *)Record;
    Serial = GetSmbiosString (Record, Type1->SerialNumber);
    if ((Serial == NULL) || (Serial[0] == 0)) {
      DEBUG ((DEBUG_WARN, "AcpiEcDispatch 59/D3: SMBIOS Type 1 serial empty\n"));
      return EFI_SUCCESS;
    }

    SerialLen = AsciiStrLen (Serial);
    if (SerialLen + 1 > ResponseMax) {
      return EFI_BUFFER_TOO_SMALL;
    }

    CopyMem (Response, Serial, SerialLen + 1);
    *ResponseLen = SerialLen + 1;
    DEBUG ((DEBUG_INFO, "AcpiEcDispatch 59/D3: serial \"%a\"\n", Serial));
    return EFI_SUCCESS;
  }
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
  OnboardingAcpiEcIoDispatchLibRegister59D3 (AcpiEc59D3SmbiosSerialHandler);

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
    "AcpiEcIoDispatchDxe: cmd 0x59 / sub 0xD0 and 0xD3 dispatch ready (ports 0x%02x data, 0x%02x cmd)\n",
    PcdGet16 (PcdAcpiEcDataPort),
    PcdGet16 (PcdAcpiEcCmdStatusPort)
    ));
  return EFI_SUCCESS;
}
