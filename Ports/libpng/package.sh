#!/usr/bin/env -S bash ../.port_include.sh
port='libpng'
version='1.6.51'
useconfigure='true'
configopts=(
    '--disable-static'
    '--enable-shared'
)
use_fresh_config_sub='true'
files=(
    "https://download.sourceforge.net/libpng/libpng-${version}.tar.gz#ac25cafc2054cda3f6f0fe22ee9fc587024b99e01d03bd72b765824e48f39021"
)
depends=(
    'zlib'
)
