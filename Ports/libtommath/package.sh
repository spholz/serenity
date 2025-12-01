#!/usr/bin/env -S bash ../.port_include.sh
port='libtommath'
version='1.3.0'
useconfigure='true'
files=(
    "https://github.com/libtom/libtommath/releases/download/v${version}/ltm-${version}.tar.xz#296272d93435991308eb73607600c034b558807a07e829e751142e65ccfa9d08"
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
