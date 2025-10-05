#!/bin/bash

set -e

SERENITY_VM_DIR=$PWD

cd $(repo --show-toplevel)
. build/envsetup.sh
lunch aosp_cf_arm64_phone-bp2a-userdebug
mmm $(realpath --relative-to="$PWD" $SERENITY_VM_DIR)
