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
FLAGS=(-D__STDC_FORMAT_MACROS=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -include "$HERE/native_inttypes.h" -march=armv7-a -mfloat-abi=soft -marm -mthumb-interwork -ffreestanding -fno-common -fno-short-enums -ffunction-sections -fdata-sections -Os)
INC=(-I"$UPSTREAM" -I"$UPSTREAM/lynx" -I"$UPSTREAM/libretro" -I"$UPSTREAM/libretro-common/include")
CORE_FLAGS=(-DFRONTEND_SUPPORTS_RGB565 -DWANT_CRC32 -DHAVE_NO_LANGEXTRA -DNDEBUG)
# Verify integer formats against the actual argument types (not just presence).
# If newlib omits PRIuPTR, select its type-correct spelling using the compiler.
for fmt in u lu; do
    if printf '#include <stdio.h>\n#include <stdint.h>\nvoid check(uintptr_t p) { printf("%%%s", p); }\n' "$fmt" |
        "$CXX" -march=armv7-a -mfloat-abi=soft -std=gnu++11 -Wformat=2 -Werror -x c++ -fsyntax-only - >/dev/null 2>&1; then
        FLAGS+=("-DHANDY_PRIUPTR=\"$fmt\"")
        break
    fi
done
printf '#include <stdio.h>\nvoid check(int64_t i,uint64_t u,uintptr_t p) { printf("%%" PRId64 " %%" PRIu64 " %%" PRIuPTR, i,u,p); }\n' |
    "$CXX" "${FLAGS[@]}" -std=gnu++11 -Wformat=2 -Werror -x c++ -fsyntax-only -
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
