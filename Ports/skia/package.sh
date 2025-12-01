#!/usr/bin/env -S bash ../.port_include.sh
port='skia'
version='f406b708b8c0d31e1bfad101fc3d1ff00e7fb19e' # Tip of the chrome/m129 branch at 2025-12-01.
useconfigure='true'
files=(
    "git+https://skia.googlesource.com/skia.git#${version}"
)
depends=(libjpeg-turbo libpng libwebp libicu expat harfbuzz freetype fontconfig zlib libicu)

configopts=(
    'is_official_build=true'
    'is_component_build=true'
    'is_debug=false'
    'skia_use_dng_sdk=false'
    'skia_use_wuffs=false'
    'skia_use_zlib=true'
    'skia_use_system_zlib=true'
    'skia_use_harfbuzz=true'
    'skia_use_fontconfig=true'
    'skia_use_icu=true'
    'skia_use_system_icu=true'
    'skia_use_freetype=true'
    'skia_use_freetype2=true'
    'skia_use_fontconfig=true'
    'extra_cflags=["-Wno-psabi", "-DSKCMS_PORTABLE"]'
    # 'extra_cflags_cc=["-DSKCMS_API=__attribute__((visibility(\"default\")))"]'

    'target_os="serenity"'
    "target_cpu=\"${SERENITY_ARCH}\""
    "cc=\"${CC}\""
    "cxx=\"${CXX}\""
    "ar=\"${AR}\""

    "host_cc=\"${HOST_CC}\""
    "host_cxx=\"${HOST_CXX}\""
    "host_ar=\"${HOST_AR}\""
)

configure() {
    run gn gen out --args="${configopts[*]}"
}

build() {
    run ninja -C out :skia :modules
}

install() {
    run mkdir -p ${SERENITY_INSTALL_ROOT}/usr/local/lib/
    run mkdir -p ${SERENITY_INSTALL_ROOT}/usr/local/include/skia/modules/

    pushd $workdir/out
    run_nocd find . -name '*.so' -exec install -Dm644 {} ${SERENITY_INSTALL_ROOT}/usr/local/lib/{} \;
    run_nocd find . -name '*.a' -exec install -Dm644 {} ${SERENITY_INSTALL_ROOT}/usr/local/lib/{} \;
    popd

    pushd $workdir/include
    run_nocd find . -name '*.h' -exec install -Dm644 {} ${SERENITY_INSTALL_ROOT}/usr/local/include/skia/{} \;
    popd

    pushd $workdir/modules
    run_nocd find . -name '*.h' -exec install -Dm644 {} ${SERENITY_INSTALL_ROOT}/usr/local/include/skia/modules/{} \;
    popd

    run find ${SERENITY_INSTALL_ROOT}/usr/local/include/skia -name '*.h' -exec sed -i 's|#include "include/|#include "|' {} \;

    cat <<EOF >${SERENITY_INSTALL_ROOT}/usr/local/lib/pkgconfig/skia.pc
prefix=/usr/local
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include/skia

Name: skia
Description: Complete 2D graphic library for drawing Text, Geometries, and Images.
URL: https://skia.org/
Version: 129
Libs: -L\${libdir} -lskia -lskcms
Cflags: -I\${includedir}
EOF
}
