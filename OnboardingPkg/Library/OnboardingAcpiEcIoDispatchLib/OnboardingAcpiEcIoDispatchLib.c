/** @file
  ACPI EC I/O dispatch library — cmd 0x59 / sub-commands 0xD0 (params) and 0xD5 (serial read).

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

STATIC ACPI_EC_DISPATCH_STATE       mState = AcpiEcDispatchStateIdle;
STATIC UINT8                        mPendingCmd;
STATIC UINT8                        mPendingSubCmd;
STATIC ONBOARDING_ACPI_EC_59_D0_HANDLER  mHandler59D0 = NULL;
STATIC UINT8                        *mParamBuffer = NULL;
STATIC UINTN                        mParamSize;
STATIC UINTN                        mParamCapacity;
STATIC CHAR8                        mSerial59D5[ONBOARDING_ACPI_EC_59_D5_SERIAL_MAX + 1];
STATIC UINTN                        mSerial59D5ReadIndex;

VOID
EFIAPI
OnboardingAcpiEcIoDispatchLibInit (
  VOID
  )
{
  mState                 = AcpiEcDispatchStateIdle;
  mPendingCmd            = 0;
  mPendingSubCmd         = 0;
  mHandler59D0           = NULL;
  mParamBuffer           = NULL;
  mParamSize             = 0;
  mParamCapacity         = 0;
  mSerial59D5[0]         = '\0';
  mSerial59D5ReadIndex   = 0;
}

VOID
EFIAPI
OnboardingAcpiEcIoDispatchLibSet59D5Serial (
  IN CONST CHAR8  *SerialAscii
  )
{
  UINTN  CopyLen;

  if (SerialAscii == NULL) {
    mSerial59D5[0] = '\0';
    return;
  }

  CopyLen = AsciiStrLen (SerialAscii);
  if (CopyLen > ONBOARDING_ACPI_EC_59_D5_SERIAL_MAX) {
    CopyLen = ONBOARDING_ACPI_EC_59_D5_SERIAL_MAX;
  }

  CopyMem (mSerial59D5, SerialAscii, CopyLen);
  mSerial59D5[CopyLen] = '\0';
  DEBUG ((DEBUG_INFO, "AcpiEcDispatch: 59/D5 serial cached (%u bytes): %a\n", CopyLen, mSerial59D5));
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
    mParamBuffer     = NULL;
    mParamSize       = 0;
    mParamCapacity   = 0;
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
  mPendingCmd    = Value;
  mPendingSubCmd = 0;
  mSerial59D5ReadIndex = 0;

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

      if (Value == ONBOARDING_ACPI_EC_SUBCMD_59_D5) {
        mState               = AcpiEcDispatchStateReadResponse;
        mSerial59D5ReadIndex = 0;
        DEBUG ((DEBUG_INFO, "AcpiEcDispatch: sub-command 0xD5, serial read ready on 0x62\n"));
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
      DEBUG ((DEBUG_VERBOSE, "AcpiEcDispatch: data 0x%02x ignored (state %u)\n", Value, mState));
      return EFI_NOT_READY;
  }
}

STATIC
EFI_STATUS
ProcessDataPortRead (
  OUT UINT8  *Value
  )
{
  CHAR8  Byte;

  if (mState != AcpiEcDispatchStateReadResponse) {
    return EFI_NOT_READY;
  }

  if (mPendingSubCmd != ONBOARDING_ACPI_EC_SUBCMD_59_D5) {
    return EFI_UNSUPPORTED;
  }

  Byte = mSerial59D5[mSerial59D5ReadIndex];
  *Value = (UINT8)Byte;

  if (Byte == '\0') {
    mState = AcpiEcDispatchStateIdle;
    DEBUG ((DEBUG_INFO, "AcpiEcDispatch: 59/D5 serial read complete\n"));
    return EFI_SUCCESS;
  }

  mSerial59D5ReadIndex++;
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

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

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
