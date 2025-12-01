#!/usr/bin/env -S bash ../.port_include.sh
port='woff2'
version='0f4d304faa1c62994536dc73510305c7357da8d4'
useconfigure='true'
files=(
    "git+https://github.com/google/woff2#${version}"
)
configopts=(
    "-DCMAKE_TOOLCHAIN_FILE=${SERENITY_BUILD_DIR}/CMakeToolchain.txt"
    '-DCMAKE_BUILD_TYPE=Release'
)
depends=(brotli)

configure() {
    run cmake -G Ninja -B build -S . "${configopts[@]}"
}

build() {
    run cmake --build build
}

install() {
    run cmake --install build
}
