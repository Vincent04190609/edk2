/** @file
  ACPI EC I/O dispatch library — cmd 0x59 sub-commands 0xD0 (write) and 0xD3 (read).

  Copyright (c) 2026, Onboarding Project. SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/OnboardingAcpiEcIoDispatchLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

typedef enum {
  AcpiEcDispatchStateIdle = 0,
  AcpiEcDispatchStateWaitSubCmd,
  AcpiEcDispatchStateCollectParams,
  AcpiEcDispatchStateReadResponse,
} ACPI_EC_DISPATCH_STATE;

STATIC ACPI_EC_DISPATCH_STATE            mState = AcpiEcDispatchStateIdle;
STATIC UINT8                             mPendingCmd;
STATIC UINT8                             mPendingSubCmd;
STATIC ONBOARDING_ACPI_EC_59_D0_HANDLER  mHandler59D0 = NULL;
STATIC ONBOARDING_ACPI_EC_59_D3_HANDLER  mHandler59D3 = NULL;
STATIC UINT8                             *mParamBuffer = NULL;
STATIC UINTN                             mParamSize;
STATIC UINTN                             mParamCapacity;
STATIC UINT8                             *mReadBuffer = NULL;
STATIC UINTN                             mReadSize;
STATIC UINTN                             mReadIndex;

STATIC
UINTN
GetMaxResponseBytes (
  VOID
  )
{
  UINTN  MaxBytes;

  MaxBytes = PcdGet16 (PcdAcpiEc59MaxParamBytes);
  if (MaxBytes == 0) {
    MaxBytes = 64;
  }

  return MaxBytes;
}

STATIC
VOID
ResetReadBuffer (
  VOID
  )
{
  if (mReadBuffer != NULL) {
    FreePool (mReadBuffer);
    mReadBuffer = NULL;
  }

  mReadSize  = 0;
  mReadIndex = 0;
}

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
  mHandler59D3     = NULL;
  mParamBuffer     = NULL;
  mParamSize       = 0;
  mParamCapacity   = 0;
  mReadBuffer      = NULL;
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
OnboardingAcpiEcIoDispatchLibRegister59D3 (
  IN ONBOARDING_ACPI_EC_59_D3_HANDLER  Handler
  )
{
  mHandler59D3 = Handler;
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

  MaxBytes = GetMaxResponseBytes ();

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
PrepareReadResponse (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       MaxBytes;
  UINTN       ResponseLen;

  ResetReadBuffer ();

  MaxBytes = GetMaxResponseBytes ();
  mReadBuffer = AllocateZeroPool (MaxBytes);
  if (mReadBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (mHandler59D3 != NULL) {
    Status = mHandler59D3 ((CHAR8 *)mReadBuffer, MaxBytes, &ResponseLen);
  } else {
    Status = OnboardingAcpiEcIoDispatchLibDefault59D3Handler ((CHAR8 *)mReadBuffer, MaxBytes, &ResponseLen);
  }

  if (EFI_ERROR (Status)) {
    ResetReadBuffer ();
    return Status;
  }

  if (ResponseLen == 0) {
    mReadBuffer[0] = 0;
    ResponseLen    = 1;
  } else if (ResponseLen > MaxBytes) {
    ResetReadBuffer ();
    return EFI_BUFFER_TOO_SMALL;
  } else if (mReadBuffer[ResponseLen - 1] != 0) {
    if (ResponseLen >= MaxBytes) {
      ResetReadBuffer ();
      return EFI_BUFFER_TOO_SMALL;
    }

    mReadBuffer[ResponseLen] = 0;
    ResponseLen++;
  }

  mReadSize  = ResponseLen;
  mReadIndex = 0;
  mState     = AcpiEcDispatchStateReadResponse;
  DEBUG ((DEBUG_INFO, "AcpiEcDispatch: 59/D3 serial number ready (%u byte(s))\n", ResponseLen));
  return EFI_SUCCESS;
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
  ResetReadBuffer ();
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

      if (Value == ONBOARDING_ACPI_EC_SUBCMD_59_D3) {
        DEBUG ((DEBUG_INFO, "AcpiEcDispatch: sub-command 0xD3, preparing serial read on 0x62\n"));
        Status = PrepareReadResponse ();
        if (EFI_ERROR (Status)) {
          mState = AcpiEcDispatchStateIdle;
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

  if (mState != AcpiEcDispatchStateReadResponse) {
    return EFI_NOT_READY;
  }

  if (mReadIndex >= mReadSize) {
    mState = AcpiEcDispatchStateIdle;
    return EFI_NOT_READY;
  }

  *Value = mReadBuffer[mReadIndex++];
  if (mReadIndex >= mReadSize) {
    mState = AcpiEcDispatchStateIdle;
  }

  return EFI_SUCCESS;
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
OnboardingAcpiEcIoDispatchLibDefault59D3Handler (
  OUT CHAR8  *Response,
  IN  UINTN   ResponseMax,
  OUT UINTN  *ResponseLen
  )
{
  if ((Response == NULL) || (ResponseMax == 0) || (ResponseLen == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Response[0]  = 0;
  *ResponseLen = 1;
  DEBUG ((DEBUG_WARN, "AcpiEcDispatch 59/D3: no handler registered, returning empty serial\n"));
  return EFI_SUCCESS;
}
