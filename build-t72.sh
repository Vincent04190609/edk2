#!/bin/bash
# T72 BIOS Build Script

set -e

echo "=== Starting T72 BIOS Build ==="
echo "Workspace: $(pwd)"
echo "Date: $(date)"
echo ""

echo "Step 1: Setting up EDK2 environment..."
source edksetup.sh BaseTools

echo ""
echo "Step 2: Starting build..."
build -a X64 -t GCC5 -b RELEASE -p OvmfPkg/OvmfPkgX64.dsc

echo ""
echo "=== Build Complete ==="
echo "Artifacts location: Build/OvmfX64/RELEASE_GCC5/FV/"
ls -lh Build/OvmfX64/RELEASE_GCC5/FV/OVMF*.fd 2>/dev/null || echo "Artifacts not found"
