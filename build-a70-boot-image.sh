#!/bin/sh

set -eux

KERNEL_BINARY=Build/aarch64/Kernel/Kernel.bin
FDT=a70q.dtb

cleanup() {
    rm -f kernel.bin.gz kernel.bin.gz+fdt
}
trap cleanup EXIT

gzip -c -n -9 -f $KERNEL_BINARY > kernel.bin.gz
cat kernel.bin.gz $FDT > kernel.bin.gz+fdt

mkbootimg --header_version 1 --kernel kernel.bin.gz+fdt -o boot.img
avbtool make_vbmeta_image --flags 2 --padding_size 4096 --output vbmeta.img

# build and flash with `Meta/serenity.sh build aarch64 && ./build-a70-boot-image.sh && heimdall flash --wait --BOOT boot.img --VBMETA vbmeta.img`
