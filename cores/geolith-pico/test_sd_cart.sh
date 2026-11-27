#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
"${CC:-cc}" ${CFLAGS:-} -O2 -Wall -Wextra cores/geolith-pico/{sd_cart,sd_cart_test,cart_io,neo_format,geo_zlib}.c emu/gba/{fat_ro,fat_exfat}.c -lz -o "$W/test"
"${CC:-cc}" ${CFLAGS:-} -O2 -Wall -Wextra cores/geolith-pico/{native_dir,native_dir_test}.c emu/gba/{fat_ro,fat_exfat}.c -o "$W/dir-test"
"${CC:-cc}" ${CFLAGS:-} -O2 -Wall -Wextra -Werror cores/geolith-pico/{native_file,native_file_test}.c emu/gba/{fat_ro,fat_exfat}.c -o "$W/file-test"
python3 - "$W" <<'PY'
import pathlib,struct,sys
w=pathlib.Path(sys.argv[1])
sizes=(0x100000,0x10000,0x10000,1,0,0x100000)
h=bytearray(4096)
h[:4]=b'NEO\x01'
struct.pack_into('<6I',h,4,*sizes)
(w/'Test Cart.NEO').write_bytes(h+b'\x5a'*sum(sizes))
(w/'bad.neo').write_bytes(bytes(4096))
PY
python3 cores/geolith-pico/fixture.py "$W/bios"
for bits in 16 32; do
    img="$W/fat$bits.img"
    truncate -s 64M "$img"
    mkfs.fat -F "$bits" "$img"
    mmd -i "$img" ::/roms ::/roms/neo ::/bios
    mcopy -i "$img" "$W/Test Cart.NEO" "$W/bad.neo" ::/roms/neo/
    mcopy -i "$img" "$W/bios/aes.zip" "$W/bios/neogeo.zip" ::/bios/
    if [[ "${GEO_TEST_ARM:-0}" == 1 ]]; then
        timeout 120s qemu-arm -L /usr/arm-linux-gnueabi "$W/test" "$img"
        timeout 120s qemu-arm -L /usr/arm-linux-gnueabi "$W/dir-test" "$img"
        timeout 120s qemu-arm -L /usr/arm-linux-gnueabi "$W/file-test" "$img"
    else
        "$W/test" "$img"
        "$W/dir-test" "$img"
        "$W/file-test" "$img"
    fi
done
