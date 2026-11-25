"""Generate original minimal 68K/Z80 loops, never real Neo Geo ROMs."""
import sys, struct, zipfile
from pathlib import Path
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
# Reset SSP in work RAM, reset PC in BIOS; 68K BRA.S -2.
# Neo Geo dumps use swapped bytes in each 16-bit word.
def swapped(b):
    r=bytearray(b);r[0::2],r[1::2]=b[1::2],b[0::2];return r
bios=bytearray(0x20000)
for i in range(0,0x100,4):
    struct.pack_into(">I",bios,i,0xc00100)
struct.pack_into(">II",bios,0,0x10ff00,0xc00100)
bios[0x100:0x102]=b"\x60\xfe"
for zipname,names in [
    ("aes.zip",["neo-po.bin","neo-epo.bin"]),
    ("neogeo.zip",["sp-u2.sp1","japan-j3.bin","sp-45.sp1","sp-s2.sp1","uni-bios_4_0.rom"])
]:
    with zipfile.ZipFile(out/zipname,"w",zipfile.ZIP_DEFLATED) as z:
        for name in names:z.writestr(name,swapped(bios))
        z.writestr("000-lo.lo",bytes(0x10000))
        z.writestr("sfix.sfix",bytes(0x20000))
        z.writestr("sm1.sm1",bytes([0x76])*0x20000)
p=bytearray(0x100000)
struct.pack_into(">II",p,0,0x10ff00,0x000100)
p[0x100:0x102]=b"\x60\xfe"
parts=[swapped(p),bytes(0x20000),bytes([0x76])*0x20000,bytes(0x80000),b"",bytes(0x200000)]
header=bytearray(4096);header[:4]=b"NEO\x01"
struct.pack_into("<6I",header,4,*map(len,parts))
header[44:58]=b"Pico test cart"
(out/"test.neo").write_bytes(header+b"".join(parts))
