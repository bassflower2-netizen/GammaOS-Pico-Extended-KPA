# Handy Pico – stage 1 (experimental)

Upstream: https://github.com/libretro/libretro-handy at bc55d462f0b2d6b073ea93dc552ebd73cec60fd1. Handy was written by K. Wilkins and maintained by its upstream contributors. The original notices remain in the upstream checkout.

This is a portable adapter for the upstream libretro core, **not** an LK-loadable blob. The independent CI runs a synthetic homebrew ROM on Linux, checks memory-backed loading, video/audio callbacks, input, reset, and same-session state round-trip, and cross-compiles the adapter for ARMv7-A soft float. No commercial ROM or BIOS is included.

Next: replace upstream VFS and libc/heap dependencies with LK-safe implementations, provide an explicit BIOS and EEPROM buffer contract, allocate a verified DRAM slot and stack, build and validate a flat blob, then wire the driver, ROM scanning, both menu families, saves, rotation, timing and controller settings. Upstream declares single-session serialization; persistence across cold boots needs separate investigation. No original system or menu is modified by this stage.
