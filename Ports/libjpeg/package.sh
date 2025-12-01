#!/usr/bin/env -S bash ../.port_include.sh
port=libjpeg
version=9f
useconfigure=true
configopts=("--disable-static" "--enable-shared")
files=(
    "https://ijg.org/files/jpegsrc.v${version}.tar.gz#04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b"
)
workdir="jpeg-$version"
