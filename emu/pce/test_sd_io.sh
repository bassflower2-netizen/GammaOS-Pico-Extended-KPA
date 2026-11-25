#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
W=$(mktemp -d)
trap 'rm -rf "$W"' EXIT
cc -O2 -Wall -Wextra -Iemu/gba -o "$W/test" emu/pce/pce_sd_io_test.c emu/pce/pce_sd_io.c emu/gba/fat_ro.c emu/gba/fat_exfat.c
python3 - "$W" <<'PY'
from pathlib import Path
import sys
w=Path(sys.argv[1])
(w/"syscard3.pce").write_bytes(bytes([0xa5])*262144)
(w/"track.bin").write_bytes(bytes((i>>11)^(i&255) for i in range(524288)))
(w/"Test Disc.cue").write_text('FILE "track.bin" BINARY\n  TRACK 01 MODE1/2048\n    INDEX 01 00:00:00\n')
PY
for bits in 16 32; do
    img="$W/fat$bits.img"
    truncate -s 64M "$img"
    mkfs.fat -F "$bits" "$img"
    mmd -i "$img" ::/roms ::/roms/pcecd ::/bios
    mcopy -i "$img" "$W/syscard3.pce" ::/bios/
    mcopy -i "$img" "$W/track.bin" "$W/Test Disc.cue" ::/roms/pcecd/
    "$W/test" "$img"
done
