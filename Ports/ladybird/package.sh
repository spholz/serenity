#!/usr/bin/env -S bash ../.port_include.sh
port='ladybird'
version='99bef81d09db455f67b2def7fce118cd2708b8fc'
useconfigure='true'
files=(
    "git+https://github.com/LadybirdBrowser/ladybird#${version}"
)
configopts=(
    "-DCMAKE_TOOLCHAIN_FILE=${SERENITY_BUILD_DIR}/CMakeToolchain.txt"
    '-DCMAKE_BUILD_TYPE=Release'
    '-DENABLE_QT=OFF'
    # '-DENABLE_GUI_TARGETS=OFF'
    '-DENABLE_LTO_FOR_RELEASE=OFF'
    '-DCMAKE_CXX_FLAGS=-DSKCMS_PORTABLE'
    '-DLAGOM_TOOL_INSTALL=ON'
    '-DCMAKE_INSTALL_PREFIX=${SERENITY_INSTALL_ROOT}/opt/'
)
depends=(woff2 libicu fontconfig simdutf skia libtommath openssl sqlite libpng libjxl ffmpeg curl libavif)

pre_configure() {
    host_env

    run cmake -S Meta/Lagom -B lagom-tools-build -GNinja -DCMAKE_INSTALL_PREFIX=lagom-tools-install -Dpackage=LagomTools -DENABLE_GUI_TARGETS=OFF -DENABLE_LTO_FOR_RELEASE=OFF -DBUILD_LAGOM_TOOLS=ON -DLAGOM_TOOL_INSTALL=ON
    run cmake --build lagom-tools-build --parallel "${MAKEJOBS}"
    run cmake --install lagom-tools-build
}

configure() {
    env
    run cmake -G Ninja -B build -S . "${configopts[@]}" -DLagomTools_DIR="${PWD}/$workdir"/lagom-tools-install/share/LagomTools
}

build() {
    run cmake --build build --parallel "${MAKEJOBS}"
}

install() {
    run cmake --install build
}
