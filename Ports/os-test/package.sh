#!/usr/bin/env -S bash ../.port_include.sh
port='os-test'
version='b56662d7'
files=(
    "https://gitlab.com/sortix/os-test/-/archive/b56662d7/os-test-b56662d7.tar.gz#66c4fc84edaea5ca2a336a93d79c4b580c3c9cfe93180066f3517e1db4440012"
)

depends=(
    'make'
)

makeopts=(
    "-j${MAKEJOBS}"
    "OS=SerenityOS"
    "CC=${CC}"
    "CC_FOR_BUILD=${HOST_CC}"
)

install_dir='/home/anon/os-test'

build() {
    run make "${makeopts[@]}" all
}

install() {
    run_nocd cp -r "${workdir}" "${SERENITY_INSTALL_ROOT}${install_dir}"
}

post_install() {
    echo -e "\033[1;35mTo run os-test, start SerenityOS, cd to ${install_dir}, and run 'make test'.\033[0m"
}
