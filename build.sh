#!/bin/bash
set -e
export DEVKITPRO=${DEVKITPRO:-/opt/devkitpro}
export PATH=$DEVKITPRO/devkitA64/bin:$DEVKITPRO/tools/bin:$PATH
PKG=$DEVKITPRO/portlibs/switch/bin/aarch64-none-elf-pkg-config
ARCH="-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE"
CF="$ARCH -O2 -ffunction-sections -D__SWITCH__ -I$DEVKITPRO/libnx/include $($PKG --cflags sdl2)"
LF="-specs=$DEVKITPRO/libnx/switch.specs $ARCH -L$DEVKITPRO/libnx/lib -L$DEVKITPRO/portlibs/switch/lib"
mkdir -p out
aarch64-none-elf-gcc $CF source/main.c -o out/fruitfall.elf $LF $($PKG --libs sdl2) -lnx -lm
nacptool --create "Fruitfall" "J" "1.0.0" out/fruitfall.nacp
elf2nro out/fruitfall.elf out/fruitfall.nro --nacp=out/fruitfall.nacp --romfsdir=romfs
ls -la out/fruitfall.nro
