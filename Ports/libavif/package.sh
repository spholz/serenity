#!/usr/bin/env -S bash ../.port_include.sh
port='libavif'
version='1.3.0'
useconfigure='true'
files=(
    "https://github.com/AOMediaCodec/libavif/archive/refs/tags/v${version}.tar.gz#0a545e953cc049bf5bcf4ee467306a2f113a75110edf59e61248873101cd26c1"
)
configopts=(
    "-DCMAKE_TOOLCHAIN_FILE=${SERENITY_BUILD_DIR}/CMakeToolchain.txt"
    '-DCMAKE_BUILD_TYPE=Release'
    '-DBUILD_SHARED_LIBS=ON'
)
depends=(libyuv)

configure() {
    run cmake -G Ninja -B build -S . "${configopts[@]}"
}

build() {
    run cmake --build build --parallel "${MAKEJOBS}"
}

install() {
    run cmake --install build
}
