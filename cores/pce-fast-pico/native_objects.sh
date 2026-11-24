#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
UPSTREAM="$(realpath "${1:?upstream checkout}")"
mkdir -p "${2:?output directory}"
OUT="$(realpath "$2")"
CC=arm-none-eabi-gcc
# The proven Lynx compatibility header works for C as well as C++.
# Generate a wrapper instead of passing quote-sensitive macros through make.
for fmt in u lu; do
    if printf '#include <stdio.h>\n#include <stdint.h>\nvoid check(uintptr_t p) { printf("%%%s", p); }\n' "$fmt" |
        "$CC" -march=armv7-a -mfloat-abi=soft -Wformat=2 -Werror -x c -fsyntax-only - >/dev/null 2>&1; then
        printf '#define HANDY_PRIUPTR "%s"\n#include "%s/cores/handy-pico/native_inttypes.h"\n' "$fmt" "$ROOT" > "$OUT/inttypes.h"
        break
    fi
done
test -s "$OUT/inttypes.h"
FLAGS="-march=armv7-a -mfloat-abi=soft -marm -ffreestanding -fno-common -fno-short-enums -ffunction-sections -fdata-sections -D__STDC_FORMAT_MACROS=1 -D__STDC_LIMIT_MACROS=1 -D__STDC_CONSTANT_MACROS=1 -include $OUT/inttypes.h"
printf '#include <stdio.h>\nvoid check(int64_t i,uint64_t u,uintptr_t p) { printf("%%" PRId64 " %%" PRIu64 " %%" PRIuPTR,i,u,p); }\n' |
    "$CC" $FLAGS -Wformat=2 -Werror -x c -fsyntax-only -
make -C "$UPSTREAM" -j2 platform=unix STATIC_LINKING=1 TARGET=pce-native.a HAVE_CDROM=0 HAVE_CHD=0 NEED_TREMOR=1 \
    CC="$CC" CXX=arm-none-eabi-g++ AR=arm-none-eabi-ar CPPFLAGS="$FLAGS"
"$CC" $FLAGS -std=c99 -Wall -Wextra -Werror -I"$UPSTREAM/libretro-common/include" \
    -c "$ROOT/cores/pce-fast-pico/pce_adapter.c" -o "$OUT/pce-adapter.o"
# Relocatable link combines all archive members, so only actual external
# dependencies remain in the report, rather than every intra-core reference.
arm-none-eabi-ld -r --whole-archive "$UPSTREAM/pce-native.a" --no-whole-archive "$OUT/pce-adapter.o" -o "$OUT/pce-combined.o"
arm-none-eabi-nm -u "$OUT/pce-combined.o" | sort -u | tee "$OUT/undefined.txt"
arm-none-eabi-size "$OUT/pce-combined.o"
