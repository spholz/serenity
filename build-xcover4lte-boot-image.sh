#!/bin/sh

set -eux

KERNEL_BINARY=Build/aarch64/Kernel/Kernel.bin
FDT=xcover4lte.dtb

cleanup() {
    rm -f dt.img boot.img-without-dtb
    sudo umount "$chroot/mnt/serenity"
    sudo rmdir "$chroot/mnt/serenity"
}
trap cleanup EXIT

# Samsung has a custom header (DTBH) for devicetrees. Use dtbTool-exynos from postmarketOS to create one.

chroot="$(pmbootstrap config work)/chroot_native"

sudo mkdir -p "$chroot/mnt/serenity"
sudo mount --bind "$PWD" "$chroot/mnt/serenity"
pmbootstrap -q chroot -- sh -c "apk add dtbtool-exynos; dtbTool-exynos --pagesize 2048 --platform 0x50a6 --subtype 0x217584da -o /mnt/serenity/dt.img /mnt/serenity/$FDT"

mkbootimg --header_version 0 --kernel $KERNEL_BINARY --pagesize 2048 -o boot.img-without-dtb
cat boot.img-without-dtb dt.img > boot.img

# No clue what this is, but it is needed for the bootloader to pass us the FDT.
printf '\x00\xc8\x05\x00' | dd of=boot.img bs=1 seek=$((0x28)) count=4 conv=notrunc

# build and flash with `Meta/serenity.sh build aarch64 && ./build-xcover4lte-boot-image.sh && heimdall flash --wait --BOOT boot.img`
