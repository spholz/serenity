#!/usr/bin/env -S bash ../.port_include.sh
port='simdutf'
version='7.7.0'
useconfigure='true'
files=(
    "https://github.com/simdutf/simdutf/archive/refs/tags/v${version}.tar.gz#0180de81a1dd48a87b8c0442ffa81734f3db91a7350914107a449935124e3c6f"
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
