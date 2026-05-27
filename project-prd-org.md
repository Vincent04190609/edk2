---
description: Product Requirements Document (PRD) - Complete product requirements for the EDK2 BIOS firmware development project
alwaysApply: true
---

# Product Requirements Document (PRD)

## 1. Product Overview

**Product Name**: EDK2 UEFI BIOS Firmware
**Project Code**: EDK2_0526
**Knowledge paths**: `.cursor/rules/project-knowledge.mdc` — edit `PROJECT_NAME`, `PROJECT_KB_ROOT` only; `PROJECT_KB_README` is derived. Current project: **Onboarding**.
**Repository**: `https://github.com/Vincent04190609/edk2.git`
**Base Framework**: TianoCore EDK II (EFI Development Kit II)
**License**: BSD-2-Clause Plus Patent License

This knowledge base contains comprehensive BIOS/UEFI development documentation:
\\
Note: Relative paths (../../) in spec references are relative to the Primary Knowledge Base Repository (d:\VibeCoding\Projects\Onboarding\), not the current source tree.
\\




This project develops and customizes UEFI BIOS firmware based on the TianoCore EDK II open-source framework, supporting firmware requirements for multi-platform hardware products.

## 2. Goals & Scope

### Core Goals
- Develop stable, secure UEFI BIOS firmware based on the EDK II framework
- Customize SMBIOS data tables to match product hardware specifications
- Implement platform-specific features (e.g., auto-boot to Setup Menu)
- Maintain compatibility and mergeability with upstream EDK II

### Product Scope
- UEFI firmware development (compliant with UEFI Specification)
- SMBIOS data table customization (compliant with SMBIOS Specification)
- ACPI table support (compliant with ACPI Specification)
- Platform CI/CD automated build and testing

## 3. Technical Architecture

### Supported Platforms & Architectures
| Platform | Architecture | Build Toolchain |
|----------|-------------|----------------|
| Windows | IA32 / X64 | VS2019 |
| Ubuntu | IA32 / X64 / ARM / AARCH64 | GCC5 |

### Core Packages
| Package | Purpose |
|---------|---------|
| MdePkg | UEFI/PI base definitions and libraries |
| MdeModulePkg | Common UEFI module implementations |
| OvmfPkg | QEMU/KVM virtual machine firmware |
| ArmPkg / ArmVirtPkg | ARM architecture support |
| SecurityPkg | Secure Boot, TPM, cryptographic features |
| CryptoPkg | Cryptographic libraries (OpenSSL / MbedTLS) |
| NetworkPkg | Network protocol stack |
| ShellPkg | UEFI Shell environment |
| UefiCpuPkg | CPU initialization and management |
| UnitTestFrameworkPkg | Unit testing framework |

### Build System
- **Build Tools**: EDK II Build System (`edksetup.bat` / `edksetup.sh`)
- **CI Tools**: Stuart (edk2-pytool-extensions)
- **CI Platform**: Azure DevOps Pipelines
- **Build Targets**: DEBUG / RELEASE / NOOPT

### External Dependencies (Submodules)
- OpenSSL (crypto), MbedTLS (crypto), Brotli (compression)
- Google Test / CMocka / Subhook (testing)
- libspdm (device security), jansson (JSON), libfdt (FDT)

## 4. Functional Requirements

### FR-1: SMBIOS Data Table Customization
- **Priority**: P0 (Required)
- Support SMBIOS Type 1 (System Information) serial number configuration
- Support SMBIOS Type 72 custom fields (serial number + BIOS version)
- Serial number format must comply with product definition (e.g., `0FEDCBA987654321`)

### FR-2: Auto-Boot to Setup Menu
- **Priority**: P1 (Important)
- Implement FWB-216 auto-boot to BIOS Setup menu functionality
- Provide a configurable toggle mechanism

### FR-3: Platform Build & CI
- **Priority**: P0 (Required)
- All code changes must pass CI build verification
- Support Windows VS2019 and Ubuntu GCC5 toolchains
- CI plugins cover: compilation check, GUID uniqueness, dependency check, coding standards, spell check, license check

### FR-4: Secure Boot Support
- **Priority**: P1 (Important)
- Integrate SecurityPkg Secure Boot functionality
- Support TPM and cryptographic modules

## 5. Non-Functional Requirements

### NFR-1: Code Quality
- Comply with EDKII Coding Standard (Uncrustify formatting)
- Pass EccCheck coding standard validation
- Character encoding must be valid Unicode (CharEncodingCheck)

### NFR-2: Cross-Platform Compatibility
- Successful builds on both Windows and Ubuntu environments
- Git line ending set to `core.autocrlf false` to avoid CR/LF issues

### NFR-3: Version Control Standards
- Use selective staging to avoid committing submodule changes
- Commit message format: `FWB-<ticket>: <description>`
- Every commit must include a `Signed-off-by` signature

## 6. Development Environment Requirements

### Required Tools
- **Python 3.x** + PIP packages: edk2-pytool-library, edk2-pytool-extensions, edk2-basetools
- **Visual Studio 2019** (Windows builds) or **GCC5** (Ubuntu builds)
- **Git** (with submodule support)
- **Node.js + cspell** (spell check CI plugin)

### Environment Setup
```bash
git config --global core.autocrlf false
git clone --recurse-submodules https://github.com/Vincent04190609/edk2.git
pip install -r pip-requirements.txt
```

## 7. Collaboration Workflow

### PR Review Protocol
- **Path A (Collaborative Fix)**: Post findings → Tag engineer → Real-time discussion → Collaborative resolution → Formal approval
- **Path B (Escalation)**: Document findings → CEO/Board approval → Implement fix
- Confirm merge strategy with DevOps engineer before merging

### Branch Strategy
- `master` is the main branch
- Feature branch format: `feature/fwb-<ticket>-<description>`

## 8. Technical Specification References

- **UEFI Spec**: See external knowledge base `UEFI.md`
- **SMBIOS Spec**: See external knowledge base `SMBIOS.md`
- **ACPI Spec**: See external knowledge base `ACPI.md`
- **EDK II Docs**: [TianoCore Wiki](https://github.com/tianocore/tianocore.github.io/wiki/EDK-II)

## 9. Milestone Tracking

| Task ID | Feature | Status |
|---------|---------|--------|
| FWB-195 | SMBIOS Type 1 serial number implementation | Complete |
| FWB-203 | SMBIOS serial number + BIOS version T72 | Complete |
| FWB-209 | Update SMBIOS serial number format | Complete |
| FWB-216 | Auto-boot to Setup Menu | Complete |
| FWB-175/177 | Git selective staging guide | Complete |
| FWB-178 | Merge back to master branch | Complete |
