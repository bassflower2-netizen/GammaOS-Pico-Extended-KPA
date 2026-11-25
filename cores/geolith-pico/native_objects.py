#!/usr/bin/env python3
"""Compile the pinned Geolith engine sources for bare-metal ARM; inventory imports."""
import pathlib, shlex, subprocess, sys

up = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
cwd = up / "libretro"
plan = subprocess.check_output(["make", "-Bn", "platform=unix", "CC=arm-none-eabi-gcc"], cwd=cwd, text=True)
objects = []
zlib = pathlib.Path(sys.argv[3]).resolve()
assert (zlib / "lib/libz.a").is_file()
# Reuse the type-checked newlib format shim already used by PCE/Handy.
compat = pathlib.Path(__file__).resolve().parent.parent / "handy-pico/native_inttypes.h"
arch = ["-march=armv7-a", "-mfloat-abi=soft", "-marm"]
for fmt in ("u", "lu"):
    check = '#include <stdio.h>\n#include <stdint.h>\nvoid check(uintptr_t p) { printf("%' + fmt + '", p); }\n'
    if subprocess.run(["arm-none-eabi-gcc", *arch, "-Wformat=2", "-Werror", "-x", "c", "-fsyntax-only", "-"], input=check, text=True, capture_output=True).returncode == 0:
        break
else:
    raise RuntimeError("No matching uintptr_t format")
shim = out / "native_inttypes.h"
shim.write_text('#define HANDY_PRIUPTR "' + fmt + '"\n#include "' + str(compat) + '"\n')
subprocess.run(["arm-none-eabi-gcc", *arch, "-include", str(shim), "-Wformat=2", "-Werror", "-x", "c", "-fsyntax-only", "-"], input='#include <stdio.h>\nvoid check(int64_t i,uint64_t u,uintptr_t p) { printf("%" PRId64 " %" PRIu64 " %" PRIuPTR,i,u,p); }\n', text=True, check=True)
for line in plan.splitlines():
    if not line.startswith("arm-none-eabi-gcc "):
        continue
    args = shlex.split(line)
    sources = [a for a in args if a.startswith(("../src/", "../deps/miniz/", "../deps/speex/", "../deps/libretro-common/", "../deps/lzma/", "../deps/zstd/")) and a.endswith(".c")]
    if "-c" not in args or len(sources) != 1:
        continue
    src = sources[0]
    obj = out / (src.removeprefix("../").replace("/", "_") + ".o")
    flags = [a for a in args[1:] if a.startswith(("-I", "-D")) and not a.startswith("-DGIT_VERSION")]
    subprocess.run(["arm-none-eabi-gcc", "-march=armv7-a", "-mfloat-abi=soft", "-marm", "-ffreestanding", "-fno-common", "-Os", "-D__STDC_FORMAT_MACROS=1", "-D__STDC_LIMIT_MACROS=1", "-D__STDC_CONSTANT_MACROS=1", "-include", str(shim), "-I" + str(zlib / "include"), *flags, "-c", src, "-o", str(obj)], cwd=cwd, check=True)
    objects.append(obj)
assert len(objects) >= 15, f"Unexpected engine source count: {len(objects)}"
combined = out / "geolith-engine.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(combined), *map(str, objects), str(zlib / "lib/libz.a")], check=True)
assert any("miniz" in p.name for p in objects), "Missing ZIP dependency"
assert any("speex" in p.name for p in objects), "Missing audio resampler"
imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(combined)], text=True)
(out / "undefined.txt").write_text(imports)
missing = {line.split()[-1] for line in imports.splitlines() if line.strip()}
assert not any(s.startswith(("mz_zip_", "geo_resampler_", "chd_", "inflate", "ZSTD_", "Lzma")) for s in missing), missing
print("ZIP and audio resampler dependencies resolved in native object")
subprocess.run(["arm-none-eabi-readelf", "-h", str(combined)], check=True)
print(f"Compiled {len(objects)} native engine objects. Unresolved imports:\n{imports}")
