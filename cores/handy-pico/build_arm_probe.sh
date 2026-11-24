#!/usr/bin/env bash
# Full core native ARM test, not a firmware blob.
set -euo pipefail
export LC_ALL=C
HERE="$(cd "$(dirname "$0")" && pwd)"
UPSTREAM="$(realpath "${1:?upstream checkout required}")"
OUT="${2:-$HERE/out-probe}"
mkdir -p "$OUT"
OUT="$(realpath "$OUT")"
CC=arm-none-eabi-gcc
CXX=arm-none-eabi-g++
# newlib's C++ inttypes.h may be included before libretro's header.
# Define the standard macro opt-ins at the compiler command line.
FLAGS=(-D__STDC_FORMAT_MACROS=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -march=armv7-a -mfloat-abi=soft -marm -mthumb-interwork -ffreestanding -fno-common -fno-short-enums -ffunction-sections -fdata-sections -Os)
INC=(-I"$UPSTREAM" -I"$UPSTREAM/lynx" -I"$UPSTREAM/libretro" -I"$UPSTREAM/libretro-common/include")
CORE_FLAGS=(-DFRONTEND_SUPPORTS_RGB565 -DWANT_CRC32 -DHAVE_NO_LANGEXTRA -DNDEBUG)
# Fail early if the toolchain has not exposed the integer formatting macros.
printf '#include <inttypes.h>\n#ifndef PRId64\n#error PRId64 missing\n#endif\n' | "$CXX" "${FLAGS[@]}" -std=gnu++11 -x c++ -E - >/dev/null
OBJS=()
for src in lynx/lynxdec.cpp lynx/cart.cpp lynx/memmap.cpp lynx/mikie.cpp lynx/ram.cpp lynx/rom.cpp lynx/susie.cpp lynx/system.cpp lynx/eeprom.cpp libretro/libretro.cpp blip/Blip_Buffer.cpp blip/Stereo_Buffer.cpp; do
    obj="$OUT/${src//\//_}.o"
    echo ">> ARM compile $src"
    "$CXX" "${FLAGS[@]}" "${INC[@]}" "${CORE_FLAGS[@]}" -std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -c "$UPSTREAM/$src" -o "$obj"
    OBJS+=("$obj")
done
for src in handy_adapter.c smoke.c native_vfs.c qemu_entry.c; do
    "$CC" "${FLAGS[@]}" "${INC[@]}" -std=gnu99 -c "$HERE/$src" -o "$OUT/$src.o"
    OBJS+=("$OUT/$src.o")
done
"$CC" "${FLAGS[@]}" "${INC[@]}" -std=gnu99 -c "$UPSTREAM/libretro-common/compat/compat_strl.c" -o "$OUT/strl.o"
OBJS+=("$OUT/strl.o")
"$CXX" "${FLAGS[@]}" -std=gnu++11 -fno-exceptions -fno-rtti -fno-threadsafe-statics -fno-use-cxa-atexit -c "$HERE/native_runtime.cpp" -o "$OUT/runtime.o"
OBJS+=("$OUT/runtime.o")
"$CC" "${FLAGS[@]}" -nostartfiles -static -Wl,-e,probe_start -Wl,--gc-sections -Wl,--no-undefined -Wl,-Map,"$OUT/handy-arm.map" "${OBJS[@]}" -Wl,--start-group -lc -lm -lgcc -lnosys -Wl,--end-group -o "$OUT/handy-arm.elf"
arm-none-eabi-readelf -h "$OUT/handy-arm.elf"
arm-none-eabi-size "$OUT/handy-arm.elf"
test -z "$(arm-none-eabi-nm -u "$OUT/handy-arm.elf")"
timeout 45s qemu-arm "$OUT/handy-arm.elf"
