#!/usr/bin/env -S bash ../.port_include.sh
port='libyuv'
version='eb6e7bb63738e29efd82ea3cf2a115238a89fa51'
useconfigure='true'
files=(
    "git+https://chromium.googlesource.com/libyuv/libyuv.git#${version}"
)
configopts=(
    "-DCMAKE_TOOLCHAIN_FILE=${SERENITY_BUILD_DIR}/CMakeToolchain.txt"
    '-DCMAKE_BUILD_TYPE=Release'
    '-DBUILD_SHARED_LIBS=ON'
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
