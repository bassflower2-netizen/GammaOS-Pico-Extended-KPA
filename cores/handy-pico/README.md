# Handy Pico – stage 1 (experimental)

Upstream: https://github.com/libretro/libretro-handy at bc55d462f0b2d6b073ea93dc552ebd73cec60fd1. Handy was written by K. Wilkins and maintained by its upstream contributors. The original notices remain in the upstream checkout.

This is a portable adapter for the upstream libretro core, **not** an LK-loadable blob. The independent CI runs a synthetic homebrew ROM on Linux, checks memory-backed loading, video/audio callbacks, input, reset, and same-session state round-trip, and cross-compiles the adapter for ARMv7-A soft float. No commercial ROM or BIOS is included.

Next: replace upstream VFS and libc/heap dependencies with LK-safe implementations, provide an explicit BIOS and EEPROM buffer contract, allocate a verified DRAM slot and stack, build and validate a flat blob, then wire the driver, ROM scanning, both menu families, saves, rotation, timing and controller settings. Upstream declares single-session serialization; persistence across cold boots needs separate investigation. No original system or menu is modified by this stage.

## Stage 2: native ARM probe

`build_arm_probe.sh` compiles all Handy C++ emulation sources and the adapter with the ARMv7-A soft-float ABI, links them statically with newlib and a bounded test heap, and runs the same synthetic smoke test via qemu-arm. The file/VFS functions explicitly reject filesystem operations. Only the test executable's separate qemu_entry.c uses Linux syscalls (stdout/stderr and exit); that file must never be included in a bootloader blob. This is not yet a verified bootloader memory map, real BIOS/EEPROM support, or a deployable .blob. CI results determine whether this experimental probe actually works.

## Stage 3: experimental flat blob
The native probe additionally links a GammaOS-compatible 20-byte image header
(magic LNX1, ABI 1), tests the exact bytes by mapping the existing armSNES overlay
0x4F000000–0x50000000 in QEMU, and exercises two game sessions via the export table.
No Linux syscall entry is linked into the blob. The proposed shared slot is not
enabled in LK yet; hardware cache, lifetime, exception and memory ownership checks
remain mandatory. The blob is intentionally not copied to the firmware SD package.

## Stage 4: cartridge / BIOS / EEPROM bridge (ABI 2)
Strict complete headered LNX cartridges and validated BS93 homebrew are accepted. Raw LYX, headerless Howard-loader cartridges and unsupported header types are deliberately rejected until separately tested. The optional 512-byte BIOS is copied and checked by upstream CRC; a bad BIOS selects HLE fallback. No BIOS is distributed. EEPROM up to 2048 bytes is copied in and exported by ABI 2 for the frontend to persist. The ARM build uses marked copies of two upstream files to flush EEPROM and read the full size. Synthetic tests check malformed inputs, bad BIOS rejection and a 2048-byte EEPROM round-trip. They do not prove commercial cartridge gameplay or actual SD persistence. No bootloader/menu integration yet.

## Stage 5: LK loader and SD packaging
A read-only header check is exposed by existing oem cardcores. The LK loader uses the existing cache-maintained core_file_load path, but is not called until a dedicated session owns the shared overlay. CI compiles the loader and includes the exact QEMU-tested ABI 2 blob in the SD image. The ROM scanner and both menus remain unchanged; no Lynx launches yet. Do not flash this experimental package before integration and hardware-test instructions.

## Stage 6: session driver (still hidden from the ROM library)
The LK now has an exclusive Lynx worker thread in the fifth existing stack slot and dispatches reserved system ID 9. It uses the existing display/audio pipeline, input mapping, Current/Classic in-game menu with fallback, three state slots and separate EEPROM files. The scanner does not produce ID 9 yet. The 75 Hz panel target, physical buttons, display, power handling and SD writes require hardware verification. The first driver does not implement netplay, cheats, achievements, rewind or fast-forward. Aspect/filter settings are session-local. CI extends the flat blob test to load a same-ROM state after close/reopen; this is not a guarantee of portability across versions or all cartridges.
