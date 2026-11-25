#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
UPSTREAM="$(realpath "${1:?upstream checkout}")"
mkdir -p "${2:?output directory}"
OUT="$(realpath "$2")"
bash "$HERE/native_objects.sh" "$UPSTREAM" "$OUT"
CC=arm-none-eabi-gcc
FLAGS=(-march=armv7-a -mfloat-abi=soft -marm -ffreestanding -fno-common -fno-short-enums -ffunction-sections -fdata-sections -include "$OUT/inttypes.h" -Os)
INC=(-I"$UPSTREAM/libretro-common/include" -I"$HERE")
compile() { "$CC" "${FLAGS[@]}" "${INC[@]}" -std=gnu99 -c "$1" -o "$2"; }
compile "$HERE/native_runtime.c" "$OUT/runtime.o"
compile "$HERE/native_vfs.c" "$OUT/vfs.o"
compile "$HERE/../handy-pico/qemu_entry.c" "$OUT/qemu.o"
compile "$HERE/smoke.c" "$OUT/smoke.o"
for src in encodings/encoding_crc32.c compat/compat_strl.c string/stdstring.c; do
    compile "$UPSTREAM/libretro-common/$src" "$OUT/${src//\//_}.o"
done
CORE=("$OUT/pce-combined.o" "$OUT/runtime.o" "$OUT/vfs.o" "$OUT/encodings_encoding_crc32.c.o" "$OUT/compat_compat_strl.c.o" "$OUT/string_stdstring.c.o")
LIBS=(-Wl,--start-group -lc -lm -lgcc -lnosys -Wl,--end-group)
LINK=(-nostartfiles -static -Wl,--gc-sections -Wl,--no-undefined)
"$CC" "${FLAGS[@]}" "${LINK[@]}" -Wl,-e,probe_start "${CORE[@]}" "$OUT/smoke.o" "$OUT/qemu.o" "${LIBS[@]}" -o "$OUT/pce-arm.elf"
test -z "$(arm-none-eabi-nm -u "$OUT/pce-arm.elf")"
arm-none-eabi-size "$OUT/pce-arm.elf"
timeout 90s qemu-arm "$OUT/pce-arm.elf"
 # CD sector decoding is checked in a native ELF before the flat ABI test.
if [ "${PCE_CD_PROBE:-1}" = 1 ]; then
    "$CC" "${FLAGS[@]}" "${INC[@]}" -I"$UPSTREAM" -std=gnu99 -DPCE_NATIVE_CD -c "$HERE/cd_smoke.c" -o "$OUT/cd-smoke.o"
    "$CC" "${FLAGS[@]}" "${LINK[@]}" -Wl,-e,probe_start "${CORE[@]}" "$OUT/cd-smoke.o" "$OUT/qemu.o" "${LIBS[@]}" -o "$OUT/pce-cd-arm.elf"
    test -z "$(arm-none-eabi-nm -u "$OUT/pce-cd-arm.elf")"
    timeout 90s qemu-arm "$OUT/pce-cd-arm.elf"
fi
compile "$HERE/blob_entry.c" "$OUT/entry.o"
"$CC" "${FLAGS[@]}" "${LINK[@]}" -Wl,-T,"$HERE/blob.ld" -Wl,-Map,"$OUT/pce-blob.map" "${CORE[@]}" "$OUT/entry.o" "${LIBS[@]}" -o "$OUT/pce-blob.elf"
test -z "$(arm-none-eabi-nm -u "$OUT/pce-blob.elf")"
arm-none-eabi-objcopy -O binary "$OUT/pce-blob.elf" "$OUT/pce-fast-pico.blob"
python3 - "$OUT/pce-fast-pico.blob" <<'PY'
import struct,sys
d=open(sys.argv[1],"rb").read()
magic,ver,entry,load,span=struct.unpack_from("<5I",d)
assert magic==0x31454350 and ver==2
assert 20<=entry<load==len(d)<=span<=0x1000000 and entry%4==0
print(f"PCE flat blob: {load} bytes, span {span}, entry +0x{entry:x}")
PY
cat > "$OUT/embed.S" <<EOF
.section .rodata.probe_blob,"a",%progbits
.balign 16
.global probe_blob_start, probe_blob_end
probe_blob_start:
.incbin "$OUT/pce-fast-pico.blob"
probe_blob_end:
.section .note.GNU-stack,"",%progbits
EOF
"$CC" -march=armv7-a -mfloat-abi=soft -c "$OUT/embed.S" -o "$OUT/embed.o"
"$CC" "${FLAGS[@]}" "${INC[@]}" -std=gnu99 -DPCE_FLAT_TEST -c "$HERE/smoke.c" -o "$OUT/flat-smoke.o"
"$CC" "${FLAGS[@]}" "${LINK[@]}" -Wl,-e,probe_start "$OUT/flat-smoke.o" "$OUT/embed.o" "$OUT/runtime.o" "$OUT/qemu.o" "${LIBS[@]}" -o "$OUT/flat-smoke.elf"
timeout 90s qemu-arm "$OUT/flat-smoke.elf"

# Test the same CD lifecycle via the exact distributed flat blob export table.
# Direct CDIF sector decoding is tested in the native ELF above.
"$CC" "${FLAGS[@]}" "${INC[@]}" -I"$UPSTREAM" -std=gnu99 -DPCE_NATIVE_CD -DPCE_FLAT_TEST -c "$HERE/cd_smoke.c" -o "$OUT/flat-cd-smoke.o"
"$CC" "${FLAGS[@]}" "${LINK[@]}" -Wl,-e,probe_start "$OUT/flat-cd-smoke.o" "$OUT/embed.o" "$OUT/runtime.o" "$OUT/qemu.o" "${LIBS[@]}" -o "$OUT/flat-cd-smoke.elf"
timeout 90s qemu-arm "$OUT/flat-cd-smoke.elf"
