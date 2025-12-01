#!/usr/bin/env -S bash ../.port_include.sh
port='libjpeg-turbo'
version='3.1.2'
useconfigure='true'
files=(
    "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/${version}/libjpeg-turbo-${version}.tar.gz#8f0012234b464ce50890c490f18194f913a7b1f4e6a03d6644179fa0f867d0cf"
)
configopts=(
    "-DCMAKE_TOOLCHAIN_FILE=${SERENITY_BUILD_DIR}/CMakeToolchain.txt"
    '-DCMAKE_BUILD_TYPE=Release'
)
depends=()

configure() {
    run cmake -G Ninja -B build -S . "${configopts[@]}"
}

build() {
    run cmake --build build --parallel "${MAKEJOBS}"
}

install() {
    run cmake --install build
}
