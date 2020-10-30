#!/bin/bash

mkdir out

export ARCH=arm64

export CROSS_COMPILE=$(pwd)/../PLATFORM/prebuilts/gcc/linux-x86/aarch64/aarch64-linux-android-4.9/bin/aarch64-linux-android-

make -C $(pwd) O=$(pwd)/out KCFLAGS=-mno-android samsung/a11q_eur_open_defconfig
make -j16 -C $(pwd) O=$(pwd)/out KCFLAGS=-mno-android CONFIG_BUILD_ARM64_DT_OVERLAY=y
 
cp out/arch/arm64/boot/Image $(pwd)/arch/arm64/boot/Image
