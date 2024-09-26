#!/bin/sh

cd Build/x86_64 || exit

rm -f serenity-uefi.img
fallocate -l 2G serenity-uefi.img
sgdisk -n 1::+128M -t 1:ef00 -n 2:: -t 2:8300 serenity-uefi.img
dev=$(losetup --find --partscan --show serenity-uefi.img)
mkfs.fat -F32 -n EFI "${dev}p1"
mkfs.ext2 -I 128 -L SerenityOS "${dev}p2"
mkdir -p mnt
mount "${dev}p2" mnt
mkdir mnt/boot
mount "${dev}p1" mnt/boot
grub-install --target=x86_64-efi --efi-directory=mnt/boot --boot-directory=mnt/boot --removable
SERENITY_SOURCE_DIR="$PWD/../.." ../../Meta/build-root-filesystem.sh

boot_uuid=$(blkid -o export /dev/loop0p1 | grep ^UUID | cut -d = -f2)
partuuid=$(blkid -o export /dev/loop0p2 | grep PARTUUID | cut -d = -f2)

cat <<EOF >mnt/boot/grub/grub.cfg
timeout=1

insmod all_video

search --no-floppy --fs-uuid --set=root ${boot_uuid}

menuentry 'SerenityOS (normal)' {
  multiboot /Kernel root=PARTUUID:${partuuid}
}

menuentry 'SerenityOS (text mode)' {
  multiboot /Kernel graphics_subsystem_mode=off root=PARTUUID:${partuuid}
}

menuentry 'SerenityOS (No ACPI)' {
  multiboot /Kernel root=PARTUUID:${partuuid} acpi=off
}

menuentry 'SerenityOS (with serial debug)' {
  multiboot /Kernel serial_debug root=PARTUUID:${partuuid}
}

menuentry 'SerenityOS (debug tty)' {
  multiboot /Kernel switch_to_tty=2 root=PARTUUID:${partuuid}
}
EOF

umount -R mnt
losetup -d "${dev}"
rmdir mnt
