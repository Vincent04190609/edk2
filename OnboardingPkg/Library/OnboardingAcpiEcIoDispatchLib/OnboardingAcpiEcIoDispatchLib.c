/** @file
  ACPI EC I/O dispatch library — cmd 0x59 (sub 0xD0 params, sub 0xD2 BIOS version).

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OnboardingAcpiEcIoDispatchLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#define ONBOARDING_ACPI_EC_59_D2_VERSION_MAX  64

typedef enum {
  AcpiEcDispatchStateIdle = 0,
  AcpiEcDispatchStateWaitSubCmd,
  AcpiEcDispatchStateCollectParams,
  AcpiEcDispatchStateReady59D2Read,
} ACPI_EC_DISPATCH_STATE;

STATIC ACPI_EC_DISPATCH_STATE       mState = AcpiEcDispatchStateIdle;
STATIC UINT8                        mPendingCmd;
STATIC UINT8                        mPendingSubCmd;
STATIC ONBOARDING_ACPI_EC_59_D0_HANDLER  mHandler59D0 = NULL;
STATIC UINT8                        *mParamBuffer = NULL;
STATIC UINTN                        mParamSize;
STATIC UINTN                        mParamCapacity;
STATIC CHAR8                        mBiosVersionReadBuf[ONBOARDING_ACPI_EC_59_D2_VERSION_MAX];
STATIC UINTN                        mBiosVersionReadLen;
STATIC UINTN                        mBiosVersionReadIndex;

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
  mParamBuffer     = NULL;
  mParamSize       = 0;
  mParamCapacity   = 0;
  mBiosVersionReadLen   = 0;
  mBiosVersionReadIndex = 0;
  SetMem (mBiosVersionReadBuf, sizeof (mBiosVersionReadBuf), 0);
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
VOID
Prepare59D2VersionBuffer (
  VOID
  )
{
  CHAR16  *Vers;
  UINTN   Index;

  Vers = (CHAR16 *)PcdGetPtr (PcdFirmwareVersionString);
  mBiosVersionReadLen   = 0;
  mBiosVersionReadIndex = 0;
  SetMem (mBiosVersionReadBuf, sizeof (mBiosVersionReadBuf), 0);

  for (Index = 0; Index < (ONBOARDING_ACPI_EC_59_D2_VERSION_MAX - 1) && Vers[Index] != L'\0'; Index++) {
    mBiosVersionReadBuf[Index] = (CHAR8)Vers[Index];
    mBiosVersionReadLen++;
  }

  mBiosVersionReadBuf[mBiosVersionReadLen] = '\0';
  mBiosVersionReadLen++;

  DEBUG ((
    DEBUG_INFO,
    "AcpiEcDispatch: 59/D2 BIOS version ready (%u byte(s) incl. NUL): \"%a\"\n",
    mBiosVersionReadLen,
    mBiosVersionReadBuf
    ));
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

  mParamBuffer = NULL;
  mParamSize   = 0;
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

  if (mState == AcpiEcDispatchStateReady59D2Read) {
    mBiosVersionReadIndex = 0;
    mBiosVersionReadLen   = 0;
  }

  ResetParamBuffer ();
  mPendingCmd = Value;
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

      if (Value == ONBOARDING_ACPI_EC_SUBCMD_59_D2) {
        Prepare59D2VersionBuffer ();
        mState = AcpiEcDispatchStateReady59D2Read;
        DEBUG ((DEBUG_INFO, "AcpiEcDispatch: sub-command 0xD2, read BIOS version from 0x62\n"));
        return EFI_SUCCESS;
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

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  DataPort = PcdGet16 (PcdAcpiEcDataPort);
  if (Port != DataPort) {
    return EFI_UNSUPPORTED;
  }

  if (mState != AcpiEcDispatchStateReady59D2Read) {
    *Value = 0;
    return EFI_NOT_READY;
  }

  if (mBiosVersionReadIndex >= mBiosVersionReadLen) {
    *Value = 0;
    mState = AcpiEcDispatchStateIdle;
    return EFI_SUCCESS;
  }

  *Value = (UINT8)mBiosVersionReadBuf[mBiosVersionReadIndex++];
  if (mBiosVersionReadIndex >= mBiosVersionReadLen) {
    mState = AcpiEcDispatchStateIdle;
    DEBUG ((DEBUG_INFO, "AcpiEcDispatch: 59/D2 version read complete\n"));
  }

  return EFI_SUCCESS;
}

/**
  Finalize parameter collection (e.g. when host signals end of command).
  Call from platform EC driver when the command sequence completes.
**/
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
