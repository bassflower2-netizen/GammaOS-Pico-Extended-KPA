#!/usr/bin/env python3
"""Compile the pinned Geolith engine sources for bare-metal ARM; inventory imports."""
import pathlib, shlex, subprocess, sys

up = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]).resolve()
out.mkdir(parents=True, exist_ok=True)
cwd = up / "libretro"
plan = subprocess.check_output(["make", "-Bn", "platform=unix", "CC=arm-none-eabi-gcc"], cwd=cwd, text=True)
objects = []
# Compile the caller-owned arena for the native target without reserving device RAM.
subprocess.run(["arm-none-eabi-gcc", "-march=armv7-a", "-mfloat-abi=soft", "-marm", "-std=c11", "-Wall", "-Wextra", "-Werror", "-ffreestanding", "-c", str(pathlib.Path(__file__).with_name("arena.c").resolve()), "-o", str(out / "arena.o")], check=True)
# Link the private newlib break hook into the relocatable probe, not the firmware.
subprocess.run(["arm-none-eabi-gcc", "-march=armv7-a", "-mfloat-abi=soft", "-marm", "-std=c11", "-Wall", "-Wextra", "-Werror", "-ffreestanding", "-DGEO_NEWLIB", "-c", str(pathlib.Path(__file__).with_name("native_heap.c").resolve()), "-o", str(out / "native_heap.o")], check=True)
subprocess.run(["arm-none-eabi-gcc", "-march=armv7-a", "-mfloat-abi=soft", "-marm", "-std=c11", "-Wall", "-Wextra", "-Werror", "-ffreestanding", "-c", str(pathlib.Path(__file__).with_name("native_align.c").resolve()), "-o", str(out / "native_align.o")], check=True)
objects.extend([out / "arena.o", out / "native_heap.o", out / "native_align.o"])
zlib = pathlib.Path(sys.argv[3]).resolve()
assert (zlib / "lib/libz.a").is_file()
# Reuse the type-checked newlib format shim already used by PCE/Handy.
compat = pathlib.Path(__file__).resolve().parent.parent / "handy-pico/native_inttypes.h"
native_headers = pathlib.Path(__file__).resolve().parent / "native_include"
arch = ["-march=armv7-a", "-mfloat-abi=soft", "-marm"]
# Compile the directory ABI independently before the expensive engine build.
subprocess.run(["arm-none-eabi-gcc", *arch, "-I" + str(native_headers),
                "-std=c11", "-Wall", "-Werror", "-x", "c", "-fsyntax-only", "-"],
               input="""#include <dirent.h>
#include <utime.h>
_Static_assert(DT_UNKNOWN == 0 && DT_DIR == 4 && DT_REG == 8 && DT_LNK == 10,
               "directory type ABI mismatch");
int (*timestamp_update)(const char *, const struct utimbuf *) = utime;
int classify(const struct dirent *e) {
    return e->d_type == DT_LNK || e->d_type == DT_DIR;
}
""", text=True, check=True)
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
    subprocess.run(["arm-none-eabi-gcc", "-march=armv7-a", "-mfloat-abi=soft", "-marm", "-ffreestanding", "-fno-common", "-Os", "-D__STDC_FORMAT_MACROS=1", "-D__STDC_LIMIT_MACROS=1", "-D__STDC_CONSTANT_MACROS=1", "-include", str(shim), "-I" + str(native_headers), "-I" + str(zlib / "include"), "-Werror=implicit-function-declaration", *flags, "-c", src, "-o", str(obj)], cwd=cwd, check=True)
    objects.append(obj)
assert len(objects) >= 15, f"Unexpected engine source count: {len(objects)}"
combined = out / "geolith-engine.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(combined), *map(str, objects), str(zlib / "lib/libz.a")], check=True)
assert any("miniz" in p.name for p in objects), "Missing ZIP dependency"
assert any("speex" in p.name for p in objects), "Missing audio resampler"
imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(combined)], text=True)
(out / "undefined.txt").write_text(imports)
missing = {line.split()[-1] for line in imports.splitlines() if line.strip()}
assert not any(s.startswith(("mz_zip_", "geo_resampler_", "chd_", "inflate", "deflate", "gz", "ZSTD_", "Lzma")) or s in {"compress", "compress2", "uncompress", "uncompress2", "crc32", "adler32"} for s in missing), missing
assert {"opendir", "readdir", "closedir", "utime"} <= missing, "Directory and timestamp ABI must remain explicit until a platform adapter is linked"
assert not {"geo_arena_init", "geo_arena_sbrk", "geo_heap_break", "_sbrk"} & missing, missing
defined = subprocess.check_output(["arm-none-eabi-nm", "--defined-only", str(combined)], text=True)
assert any(line.split()[-1:] == ["_sbrk"] for line in defined.splitlines()), "Missing newlib heap hook"
print("ZIP and audio resampler dependencies resolved; directory ABI remains unresolved")
subprocess.run(["arm-none-eabi-readelf", "-h", str(combined)], check=True)
print(f"Compiled {len(objects)} native engine objects. Unresolved imports:\n{imports}")

# Link a separate allocator probe with real newlib, preserving the engine inventory.
# Relocatable linking deliberately leaves unrelated platform syscalls visible.
probe = out / "newlib-allocator-probe.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-std=c11", "-Wall", "-Wextra", "-Werror", "-ffreestanding", "-c", str(pathlib.Path(__file__).with_name("newlib_allocator_probe.c").resolve()), "-o", str(probe)], check=True)
linked = out / "newlib-allocator-linked.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-nostdlib", "-Wl,-r", "-o", str(linked), str(probe), str(out / "arena.o"), str(out / "native_heap.o"), str(out / "native_align.o"), "-Wl,--start-group", "-lc", "-lgcc", "-Wl,--end-group"], check=True)
probe_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(linked)], text=True)
probe_missing = {line.split()[-1] for line in probe_imports.splitlines() if line.strip()}
assert not {"malloc", "calloc", "realloc", "free", "posix_memalign", "memalign", "_malloc_r", "_calloc_r", "_realloc_r", "_free_r", "_sbrk", "_sbrk_r"} & probe_missing, probe_missing
probe_defined = subprocess.check_output(["arm-none-eabi-nm", "--defined-only", str(linked)], text=True)
assert {"calloc", "realloc", "free", "_sbrk", "_sbrk_r", "geo_newlib_probe"} <= {line.split()[-1] for line in probe_defined.splitlines() if line.strip()}, probe_defined
(out / "newlib-allocator-imports.txt").write_text(probe_imports)
print("Real newlib allocator link probe passed (not executed):", probe_imports)

# Link the real SD staging stack with the engine in a single native object.
# Keep platform imports visible; this is not a flashable firmware image.
root = pathlib.Path(__file__).resolve().parent
bridge = []
for source in ("neo_format.c", "cart_io.c", "sd_cart.c", "geo_zlib.c"):
    obj = out / ("bridge_" + source + ".o")
    subprocess.run(["arm-none-eabi-gcc", *arch, "-std=c11", "-Wall", "-Wextra",
                    "-Werror", "-ffreestanding", "-I" + str(zlib / "include"),
                    "-c", str(root / source), "-o", str(obj)], check=True)
    bridge.append(obj)
session = out / "geolith-session-linked.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(session), str(combined),
                *map(str, bridge), str(zlib / "lib/libz.a")], check=True)
session_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(session)], text=True)
session_missing = {line.split()[-1] for line in session_imports.splitlines() if line.strip()}
session_defined = {line.split()[-1] for line in subprocess.check_output(
    ["arm-none-eabi-nm", "--defined-only", str(session)], text=True).splitlines() if line.strip()}
assert {"geo_sd_session_handoff", "geo_sd_cart_stage", "geo_zip_zlib_raw_inflate"} <= session_defined
assert not {"inflate", "inflateInit2_", "inflateEnd", "geo_sd_bios_set_load",
            "geo_sd_cart_stage", "geo_cart_load", "pico_neo_validate"} & session_missing
assert {"fat_open", "fat_read"} <= session_missing
(out / "session-imports.txt").write_text(session_imports)
print("Native engine and SD session relocatable link passed; FAT/platform imports remain explicit")

# Resolve the actual firmware FAT reader, including its exFAT dependency.
fat_objects = []
for source in ("fat_ro.c", "fat_exfat.c"):
    obj = out / ("session_" + source + ".o")
    subprocess.run(["arm-none-eabi-gcc", *arch, "-std=gnu11", "-Os",
                    "-ffreestanding", "-Werror=implicit-function-declaration",
                    "-c", str(root.parent.parent / "emu/gba" / source),
                    "-o", str(obj)], check=True)
    fat_objects.append(obj)
storage = out / "geolith-storage-linked.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(storage), str(session),
                *map(str, fat_objects)], check=True)
storage_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(storage)], text=True)
storage_missing = {line.split()[-1] for line in storage_imports.splitlines() if line.strip()}
assert not any(s.startswith(("fat_", "exfat_")) for s in storage_missing), storage_missing
storage_defined = {line.split()[-1] for line in subprocess.check_output(
    ["arm-none-eabi-nm", "--defined-only", str(storage)], text=True).splitlines() if line.strip()}
assert {"fat_mount", "fat_open", "fat_read", "geo_sd_session_handoff"} <= storage_defined
(out / "storage-imports.txt").write_text(storage_imports)
print("Native Geolith session and firmware FAT reader link passed")

# Resolve real newlib/libm/compiler helpers for the entire native storage
# engine, rather than only the isolated allocator. Keep platform syscalls
# unresolved: no dummy success callbacks or executable firmware here.
libc_linked = out / "geolith-libc-linked.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-nostdlib", "-Wl,-r",
                "-o", str(libc_linked), str(storage),
                "-Wl,--start-group", "-lc", "-lm", "-lgcc",
                "-Wl,--end-group"], check=True)
libc_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(libc_linked)], text=True)
libc_missing = {line.split()[-1] for line in libc_imports.splitlines() if line.strip()}
assert not {"malloc", "calloc", "realloc", "free", "memcpy", "memset",
            "fopen", "fread", "fclose", "printf", "sqrt", "pow", "_sbrk", "posix_memalign", "memalign"} & libc_missing, libc_missing
assert not any(s.startswith(("fat_", "exfat_", "__aeabi_")) for s in libc_missing), libc_missing
assert {"opendir", "readdir", "closedir", "utime"} <= libc_missing
(out / "libc-imports.txt").write_text(libc_imports)
print("Native storage engine linked with real libc/libm/libgcc; platform imports remain:", libc_imports)

# Resolve real read-only directory enumeration against the mounted FAT volume.
directory = out / "native_dir.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-std=c11", "-Wall", "-Wextra",
                "-Werror", "-ffreestanding", "-c", str(root / "native_dir.c"),
                "-o", str(directory)], check=True)
directory_linked = out / "geolith-directory-linked.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-nostdlib", "-Wl,-r",
                "-o", str(directory_linked), str(libc_linked), str(directory),
                "-Wl,--start-group", "-lc", "-lgcc", "-Wl,--end-group"], check=True)
directory_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(directory_linked)], text=True)
directory_missing = {line.split()[-1] for line in directory_imports.splitlines() if line.strip()}
assert not {"opendir", "readdir", "closedir", "fat_opendir", "fat_readdir"} & directory_missing
assert "utime" in directory_missing
print("Native FAT directory adapter linked; remaining imports:", directory_imports)

# Resolve the actual read-only newlib file calls, without providing fake writes.
file_obj = out / "native_file.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-std=gnu11", "-Wall", "-Wextra",
                "-Werror", "-ffreestanding", "-DGEO_NEWLIB_SYSCALLS",
                "-c", str(root / "native_file.c"), "-o", str(file_obj)], check=True)
file_linked = out / "geolith-file-linked.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-nostdlib", "-Wl,-r",
                "-o", str(file_linked), str(directory_linked), str(file_obj),
                "-Wl,--start-group", "-lc", "-lgcc", "-Wl,--end-group"], check=True)
file_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(file_linked)], text=True)
file_missing = {line.split()[-1] for line in file_imports.splitlines() if line.strip()}
assert not {"_open", "_read", "_close", "_lseek", "_fstat"} & file_missing, file_missing
assert {"_write", "utime"} <= file_missing, file_missing
(out / "file-imports.txt").write_text(file_imports)
print("Native FAT file adapter linked; remaining imports:", file_imports)

# Compile the dedicated persistence boundary for the target ABI. The actual
# LK FAT writer requires device kernel locks, so do not substitute fake locks.
save_obj = out / "sd_save.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-std=c11", "-Wall", "-Wextra",
                "-Werror", "-ffreestanding", "-c", str(root / "sd_save.c"),
                "-o", str(save_obj)], check=True)
save_linked = out / "geolith-save-linked.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(save_linked),
                str(file_linked), str(save_obj)], check=True)
save_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(save_linked)], text=True)
save_missing = {line.split()[-1] for line in save_imports.splitlines() if line.strip()}
assert {"fat_wr_put", "fat_wr_mkpath"} <= save_missing, save_missing
assert not {"fat_open", "fat_read"} & save_missing, save_missing
print("Native save boundary linked; real LK writer binding remains open")

# Compile the actual firmware writer with LK's real ARM headers and preserve
# its synchronization imports. Never execute this relocatable object on Linux:
# IRQ controls and LK mutexes belong to the firmware kernel.
firmware_root = root.parent.parent
writer = out / "session_fat_wr.c.o"
subprocess.run(["arm-none-eabi-gcc", *arch, "-std=gnu11", "-Os",
                "-ffreestanding", "-fno-builtin", "-DARCH_ARM=1", "-DDEBUGLEVEL=0",
                "-I" + str(firmware_root / "include"),
                "-I" + str(firmware_root / "arch/arm/include"),
                "-Werror=implicit-function-declaration",
                "-c", str(firmware_root / "emu/gba/fat_wr.c"), "-o", str(writer)], check=True)
writer_linked = out / "geolith-writer-linked.o"
subprocess.run(["arm-none-eabi-ld", "-r", "-o", str(writer_linked),
                str(save_linked), str(writer)], check=True)
writer_imports = subprocess.check_output(["arm-none-eabi-nm", "-u", str(writer_linked)], text=True)
writer_missing = {line.split()[-1] for line in writer_imports.splitlines() if line.strip()}
writer_defined = {line.split()[-1] for line in subprocess.check_output(
    ["arm-none-eabi-nm", "--defined-only", str(writer_linked)], text=True).splitlines() if line.strip()}
assert {"geo_sd_save_put", "geo_sd_save_get", "fat_wr_put", "fat_wr_mkpath",
        "fat_wr_delete"} <= writer_defined, writer_defined
assert not any(s.startswith(("fat_", "exfat_")) for s in writer_missing), writer_missing
assert {"mutex_init", "mutex_acquire", "mutex_release", "critical_section_count",
        "arch_disable_ints", "arch_enable_ints"} <= writer_missing, writer_missing
(out / "writer-imports.txt").write_text(writer_imports)
print("Actual LK FAT writer linked with native Geolith; kernel synchronization imports preserved:", writer_imports)
