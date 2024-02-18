#!/usr/bin/env -S bash ../.port_include.sh
port='binutils'
version='2.42'
useconfigure='true'
use_fresh_config_sub='true'
configopts=(
    "--target=${SERENITY_ARCH}-pc-serenity"
    "--with-sysroot=/"
    "--with-build-sysroot=${SERENITY_INSTALL_ROOT}"
    "--disable-werror"
    "--disable-gdb"
    "--disable-nls"
    "--enable-libiberty"
)
files=(
    "https://ftpmirror.gnu.org/gnu/binutils/binutils-${version}.tar.xz#f6e4d41fd5fc778b06b7891457b3620da5ecea1006c6a4a41ae998109f85a800"
)
depends=(
    'zlib'
    'zstd'
)

export ac_cv_func_getrusage=no

install() {
    run make DESTDIR=${SERENITY_INSTALL_ROOT} "${installopts[@]}" install
    run_nocd cp ${workdir}/include/libiberty.h ${SERENITY_INSTALL_ROOT}/usr/local/include
    run_nocd cp ${workdir}/libiberty/libiberty.a ${SERENITY_INSTALL_ROOT}/usr/local/lib
}
