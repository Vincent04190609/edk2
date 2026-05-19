##
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[PcdsFixedAtBuild.common]
  # SMBIOS Manufacturer name for T36 test BIOS
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"Vincent Inc"

!ifdef $(FIRMWARE_VER)
  gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"$(FIRMWARE_VER)"
!endif
