/** @file
  ACPI EC I/O dispatch library — cmd 0x59 / sub-commands 0xD0 and 0xD1.

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OnboardingAcpiEcIoDispatchLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#define ACPI_EC_READ_RESPONSE_MAX  64

typedef enum {
  AcpiEcDispatchStateIdle = 0,
  AcpiEcDispatchStateWaitSubCmd,
  AcpiEcDispatchStateCollectParams,
} ACPI_EC_DISPATCH_STATE;

STATIC ACPI_EC_DISPATCH_STATE            mState = AcpiEcDispatchStateIdle;
STATIC UINT8                             mPendingCmd;
STATIC UINT8                             mPendingSubCmd;
STATIC ONBOARDING_ACPI_EC_59_D0_HANDLER  mHandler59D0 = NULL;
STATIC ONBOARDING_ACPI_EC_59_D1_HANDLER  mHandler59D1 = NULL;
STATIC UINT8                             *mParamBuffer = NULL;
STATIC UINTN                             mParamSize;
STATIC UINTN                             mParamCapacity;
STATIC UINT8                             mReadBuffer[ACPI_EC_READ_RESPONSE_MAX];
STATIC UINTN                             mReadSize;
STATIC UINTN                             mReadIndex;

VOID
EFIAPI
OnboardingAcpiEcIoDispatchLibInit (
  VOID
  )
{
  mState           = AcpiEcDispatchStateIdle;
  mPendingCmd      = 0;
  mPendingSubCmd   = 0;
  mHandler59D0     = NULL;
  mHandler59D1     = NULL;
  mParamBuffer     = NULL;
  mParamSize       = 0;
  mParamCapacity   = 0;
  mReadSize        = 0;
  mReadIndex       = 0;
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibRegister59D0 (
  IN ONBOARDING_ACPI_EC_59_D0_HANDLER  Handler
  )
{
  mHandler59D0 = Handler;
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibRegister59D1 (
  IN ONBOARDING_ACPI_EC_59_D1_HANDLER  Handler
  )
{
  mHandler59D1 = Handler;
  return EFI_SUCCESS;
}

STATIC
VOID
ResetReadResponse (
  VOID
  )
{
  mReadSize  = 0;
  mReadIndex = 0;
}

STATIC
VOID
ResetParamBuffer (
  VOID
  )
{
  if (mParamBuffer != NULL) {
    FreePool (mParamBuffer);
    mParamBuffer   = NULL;
    mParamSize     = 0;
    mParamCapacity = 0;
  }
}

STATIC
EFI_STATUS
EnsureParamCapacity (
  VOID
  )
{
  UINTN  MaxBytes;

  MaxBytes = PcdGet16 (PcdAcpiEc59MaxParamBytes);
  if (MaxBytes == 0) {
    MaxBytes = 64;
  }

  if (mParamBuffer == NULL) {
    mParamBuffer = AllocateZeroPool (MaxBytes);
    if (mParamBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    mParamCapacity = MaxBytes;
    mParamSize     = 0;
  }

  if (mParamSize >= mParamCapacity) {
    return EFI_BUFFER_TOO_SMALL;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
StageReadResponse (
  IN CONST CHAR8  *VersionAscii,
  IN UINTN        VersionLength
  )
{
  UINTN  CopyLen;

  ResetReadResponse ();

  if ((VersionAscii == NULL) || (VersionLength == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  CopyLen = VersionLength;
  if (CopyLen >= ACPI_EC_READ_RESPONSE_MAX) {
    CopyLen = ACPI_EC_READ_RESPONSE_MAX - 1;
  }

  CopyMem (mReadBuffer, VersionAscii, CopyLen);
  mReadBuffer[CopyLen] = '\0';
  mReadSize            = CopyLen + 1;
  mReadIndex           = 0;

  DEBUG ((
    DEBUG_INFO,
    "AcpiEcDispatch 59/D1: staged %u byte(s) for port 0x62 read\n",
    mReadSize
    ));
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
Dispatch59D1 (
  VOID
  )
{
  EFI_STATUS  Status;
  CHAR8       VersionAscii[ACPI_EC_READ_RESPONSE_MAX];
  UINTN       VersionLength;

  if (mHandler59D1 != NULL) {
    Status = mHandler59D1 (VersionAscii, sizeof (VersionAscii), &VersionLength);
  } else {
    Status = OnboardingAcpiEcIoDispatchLibDefault59D1Handler (
               VersionAscii,
               sizeof (VersionAscii),
               &VersionLength
               );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  return StageReadResponse (VersionAscii, VersionLength);
}

STATIC
EFI_STATUS
DispatchCollectedParams (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT8       *Params;
  UINTN       Size;

  if ((mPendingCmd != ONBOARDING_ACPI_EC_CMD_VENDOR_59) ||
      (mPendingSubCmd != ONBOARDING_ACPI_EC_SUBCMD_59_D0))
  {
    return EFI_UNSUPPORTED;
  }

  Params = mParamBuffer;
  Size   = mParamSize;

  mParamBuffer   = NULL;
  mParamSize     = 0;
  mParamCapacity = 0;

  if (mHandler59D0 != NULL) {
    Status = mHandler59D0 (Params, Size);
  } else {
    Status = OnboardingAcpiEcIoDispatchLibDefault59D0Handler (Params, Size);
  }

  if (Params != NULL) {
    FreePool (Params);
  }

  return Status;
}

STATIC
EFI_STATUS
ProcessCmdPortWrite (
  IN UINT8  Value
  )
{
  if (mState == AcpiEcDispatchStateCollectParams) {
    (VOID)DispatchCollectedParams ();
  }

  ResetParamBuffer ();
  ResetReadResponse ();
  mPendingCmd    = Value;
  mPendingSubCmd = 0;

  if (Value == ONBOARDING_ACPI_EC_CMD_VENDOR_59) {
    mState = AcpiEcDispatchStateWaitSubCmd;
    DEBUG ((DEBUG_INFO, "AcpiEcDispatch: cmd 0x59 on port 0x66, waiting sub-command on 0x62\n"));
    return EFI_SUCCESS;
  }

  mState = AcpiEcDispatchStateIdle;
  DEBUG ((DEBUG_VERBOSE, "AcpiEcDispatch: unhandled cmd 0x%02x on port 0x66\n", Value));
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
ProcessDataPortWrite (
  IN UINT8  Value
  )
{
  EFI_STATUS  Status;

  switch (mState) {
    case AcpiEcDispatchStateWaitSubCmd:
      mPendingSubCmd = Value;
      if (Value == ONBOARDING_ACPI_EC_SUBCMD_59_D0) {
        mState = AcpiEcDispatchStateCollectParams;
        DEBUG ((DEBUG_INFO, "AcpiEcDispatch: sub-command 0xD0, collecting params on 0x62\n"));
        return EFI_SUCCESS;
      }

      if (Value == ONBOARDING_ACPI_EC_SUBCMD_59_D1) {
        Status = Dispatch59D1 ();
        mState = AcpiEcDispatchStateIdle;
        if (!EFI_ERROR (Status)) {
          DEBUG ((DEBUG_INFO, "AcpiEcDispatch: sub-command 0xD1, BIOS version ready on 0x62 read\n"));
        }

        return Status;
      }

      DEBUG ((DEBUG_WARN, "AcpiEcDispatch: cmd 0x59 unknown sub-command 0x%02x\n", Value));
      mState = AcpiEcDispatchStateIdle;
      return EFI_UNSUPPORTED;

    case AcpiEcDispatchStateCollectParams:
      Status = EnsureParamCapacity ();
      if (Status == EFI_BUFFER_TOO_SMALL) {
        (VOID)DispatchCollectedParams ();
        mState = AcpiEcDispatchStateIdle;
        return EFI_BUFFER_TOO_SMALL;
      }

      if (EFI_ERROR (Status)) {
        mState = AcpiEcDispatchStateIdle;
        return Status;
      }

      mParamBuffer[mParamSize++] = Value;
      if (mParamSize >= mParamCapacity) {
        Status = DispatchCollectedParams ();
        mState = AcpiEcDispatchStateIdle;
        return Status;
      }

      return EFI_SUCCESS;

    default:
      DEBUG ((DEBUG_VERBOSE, "AcpiEcDispatch: data 0x%02x ignored (state idle)\n", Value));
      return EFI_NOT_READY;
  }
}

STATIC
EFI_STATUS
ProcessDataPortRead (
  OUT UINT8  *Value
  )
{
  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((mReadSize == 0) || (mReadIndex >= mReadSize)) {
    return EFI_NOT_READY;
  }

  *Value = mReadBuffer[mReadIndex++];
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibProcessWrite (
  IN UINT16  Port,
  IN UINT8   Value
  )
{
  UINT16  DataPort;
  UINT16  CmdPort;

  DataPort = PcdGet16 (PcdAcpiEcDataPort);
  CmdPort  = PcdGet16 (PcdAcpiEcCmdStatusPort);

  if (Port == CmdPort) {
    return ProcessCmdPortWrite (Value);
  }

  if (Port == DataPort) {
    return ProcessDataPortWrite (Value);
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibProcessRead (
  IN  UINT16  Port,
  OUT UINT8   *Value
  )
{
  UINT16  DataPort;

  DataPort = PcdGet16 (PcdAcpiEcDataPort);
  if (Port != DataPort) {
    return EFI_UNSUPPORTED;
  }

  return ProcessDataPortRead (Value);
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibFlush (
  VOID
  )
{
  EFI_STATUS  Status;

  if (mState != AcpiEcDispatchStateCollectParams) {
    return EFI_NOT_READY;
  }

  Status = DispatchCollectedParams ();
  mState = AcpiEcDispatchStateIdle;
  return Status;
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibDefault59D0Handler (
  IN CONST UINT8  *ParamData,
  IN UINTN        ParamSize
  )
{
  UINTN  Index;

  DEBUG ((DEBUG_INFO, "AcpiEcDispatch 59/D0: %u param byte(s)\n", ParamSize));
  for (Index = 0; Index < ParamSize; Index++) {
    DEBUG ((DEBUG_INFO, "  [%u] = 0x%02x\n", Index, ParamData[Index]));
  }

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
OnboardingAcpiEcIoDispatchLibDefault59D1Handler (
  OUT CHAR8   *VersionAscii,
  IN  UINTN   BufferSize,
  OUT UINTN   *VersionLength
  )
{
  CHAR16  *VersionWide;
  UINTN   Length;

  if ((VersionAscii == NULL) || (VersionLength == NULL) || (BufferSize == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  VersionWide = (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString);
  if (VersionWide == NULL) {
    return EFI_NOT_FOUND;
  }

  Length = 0;
  while ((VersionWide[Length] != L'\0') && (Length < BufferSize - 1)) {
    VersionAscii[Length] = (CHAR8)VersionWide[Length];
    Length++;
  }

  VersionAscii[Length] = '\0';
  *VersionLength       = Length;

  DEBUG ((DEBUG_INFO, "AcpiEcDispatch 59/D1: BIOS version \"%a\"\n", VersionAscii));
  return EFI_SUCCESS;
}
