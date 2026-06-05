#!/usr/bin/env bash
# Boot built OVMF in QEMU and dump SMBIOS Type 1 / Type 2 via a tiny Linux initramfs.
# Requires: docker, uefi-edk2-golden:latest, built OVMF at Build/OvmfX64/DEBUG_GCC5/FV/

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
FV_DIR="${FV_DIR:-$ROOT/Build/OvmfX64/DEBUG_GCC5/FV}"
WORK="$ROOT/qemu-smbios-test"

if [[ ! -f "$FV_DIR/OVMF_CODE.fd" || ! -f "$FV_DIR/OVMF_VARS.fd" ]]; then
  echo "Missing OVMF firmware in $FV_DIR" >&2
  exit 1
fi

docker run --rm \
  -v "$FV_DIR:/fv:ro" \
  -v "$WORK:/work" \
  uefi-edk2-golden:latest \
  /bin/bash -c '
    set -euo pipefail
    export DEBIAN_FRONTEND=noninteractive
    apt-get update -qq
    apt-get install -y -qq qemu-system-x86 busybox-static cpio gzip linux-image-virtual >/dev/null

    INITRAMFS=/work/initramfs-build
    rm -rf "$INITRAMFS"
    mkdir -p "$INITRAMFS"/{bin,proc,sys,dev,tmp,run}
    BB=$(dpkg -L busybox-static | grep bin/busybox | head -1)
    cp "$BB" "$INITRAMFS/bin/busybox"
    cd "$INITRAMFS/bin"
    for c in sh mount umount sleep echo cat hexdump; do ln -s busybox "$c"; done

    cat > "$INITRAMFS/init" << "INIT"
#!/bin/sh
mount -t proc proc /proc
mount -t sysfs sysfs /sys
[ -e /dev/console ] || mknod /dev/console c 5 1
mount -t devtmpfs devtmpfs /dev 2>/dev/null || mount -t tmpfs tmpfs /dev

echo "===== Linux DMI summary ====="
dmesg | grep -E "DMI:|SMBIOS|efi: EFI" || true

echo "===== Raw SMBIOS table (first 512 bytes) ====="
if [ -e /sys/firmware/dmi/tables/DMI ]; then
  hexdump -C /sys/firmware/dmi/tables/DMI | head -32
else
  echo "No /sys/firmware/dmi/tables/DMI"
fi

echo "===== Parsed strings (Type 1 / Type 2 markers) ====="
if [ -e /sys/firmware/dmi/tables/DMI ]; then
  strings /sys/firmware/dmi/tables/DMI | grep -E "^(QEMU|Vibe-Factory|OVMF|Standard PC|pc-q35|[0-9A-F]{16}|T[0-9]+)" || true
fi

echo "===== done ====="
echo o > /proc/sysrq-trigger
INIT
    chmod +x "$INITRAMFS/init"
    cd "$INITRAMFS" && find . | cpio -o -H newc | gzip > /work/smbios-initramfs.cpio.gz

    VMLINUZ=$(ls /boot/vmlinuz-* | sort -V | tail -1)
    cp /fv/OVMF_VARS.fd /tmp/OVMF_VARS.fd

    echo "Using firmware: /fv/OVMF_CODE.fd + /tmp/OVMF_VARS.fd"
    echo "Using kernel:   $VMLINUZ"
    echo "Log: /work/smbios-dump.log"

    timeout 120 qemu-system-x86_64 \
      -machine q35,accel=tcg \
      -cpu qemu64 \
      -smp 1 \
      -m 1024 \
      -fw_cfg name=opt/org.tianocore/X-Cpuhp-Bugcheck-Override,string=yes \
      -drive if=pflash,format=raw,readonly=on,file=/fv/OVMF_CODE.fd \
      -drive if=pflash,format=raw,file=/tmp/OVMF_VARS.fd \
      -device virtio-rng-pci \
      -net none \
      -display none \
      -serial mon:stdio \
      -kernel "$VMLINUZ" \
      -initrd /work/smbios-initramfs.cpio.gz \
      -append "console=ttyS0 rdinit=/init" \
      2>&1 | tee /work/smbios-dump.log

    echo
    echo "======== SMBIOS excerpt ========"
    sed -n "/===== Linux DMI summary =====/,/===== done =====/p" /work/smbios-dump.log
  '
