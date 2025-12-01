#!/usr/bin/env -S bash ../.port_include.sh
port='libtiff'
version='4.7.1'
files=(
    "http://download.osgeo.org/libtiff/tiff-${version}.tar.xz#b92017489bdc1db3a4c97191aa4b75366673cb746de0dce5d7a749d5954681ba"
)
useconfigure='true'

configopts=(
    "-DCMAKE_TOOLCHAIN_FILE=${SERENITY_BUILD_DIR}/CMakeToolchain.txt"
    '-DWEBP_BUILD_EXTRAS=OFF'
    '-DWEBP_BUILD_VWEBP=OFF'
    '-DCMAKE_BUILD_TYPE=Release'
    '-Dwebp=OFF' # Avoid a circular dependency between libwebp and libtiff.
)

workdir="tiff-${version}"
depends=(
    'libjpeg'
    'xz'
    'zstd'
)

configure() {
    run cmake -G Ninja -B build -S . "${configopts[@]}"
}

build() {
    run cmake --build build --parallel "${MAKEJOBS}"
}

install() {
    run cmake --install build
}
