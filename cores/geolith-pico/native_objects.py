#!/usr/bin/env python3
"""Compile the pinned Geolith engine sources for bare-metal ARM; inventory imports."""
import pathlib, shlex, subprocess, sys

up = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
cwd = up / "libretro"
plan = subprocess.check_output(["make", "-Bn", "platform=unix", "CC=arm-none-eabi-gcc"], cwd=cwd, text=True)
objects = []
for line in plan.splitlines():
    if not line.startswith("arm-none-eabi-gcc "):
        continue
    args = shlex.split(line)
    sources = [a for a in args if a.startswith("../src/") and a.endswith(".c")]
    if "-c" not in args or len(sources) != 1:
        continue
    src = sources[0]
    obj = out / (src.removeprefix("../src/").replace("/", "_") + ".o")
    flags = [a for a in args[1:] if a.startswith(("-I", "-D")) and not a.startswith("-DGIT_VERSION")]
    subprocess.run(["arm-none-eabi-gcc", "-march=armv7-a", "-mfloat-abi=soft", "-marm", "-ffreestanding", "-fno-common", "-Os", *flags, "-c", src, "-o", str(obj)], cwd=cwd, check=True)
    objects.append(obj)
assert len(objects) >= 15, f"Unexpected engine source count: {len(objects)}"
combined = out / "geolith-engine.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(combined), *map(str, objects)], check=True)
imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(combined)], text=True)
(out / "undefined.txt").write_text(imports)
subprocess.run(["arm-none-eabi-readelf", "-h", str(combined)], check=True)
print(f"Compiled {len(objects)} native engine objects. Unresolved imports:\n{imports}")
