# PC Engine / TurboGrafx-16 — experimental native port

Pinned upstream: https://github.com/libretro/beetle-pce-fast-libretro/tree/4cf066e22533186d0cfad85a0fec2aced93e4e29 (GPL-2.0). The original notices stay in the upstream checkout. New adapter and test code in this directory use GPL-2.0-only.

First stage: a portable, memory-backed HuCard adapter for RGB565, 44.1 kHz audio, libretro joypad input, serialization and battery-backed RAM. The independent CI tests an original synthetic HuC6280 ROM on Linux x86-64 and Linux ARMv7 soft-float via QEMU, and attempts to compile the core objects with arm-none-eabi/newlib. The latter is only an object archive, not a fully linked bare-metal executable; it inventories unresolved dependencies for the next stage. No commercial ROM, BIOS or CD image is included.

The initial adapter accepts 8 KiB-aligned HuCards up to 2.5 MiB, with optional 512-byte copier header. The host build disables CHD, physical CD access and Tremor for the initial probe. PC Engine CD / Super CD-ROM requires separate file/sector access and BIOS tests; it is not implemented by this adapter.

Next: resolve native filesystem/libc dependencies, link and execute a flat ARM blob with the proven Lynx runtime pattern, add LK loader and session, then menu/ROM/save paths. Do not advertise PC Engine in the menu until the launch path is wired. Hardware testing is deferred by the project owner until all requested systems have been integrated.

Known successful Lynx integration baseline: 32fd3b48df82447370a04ac97617b67b928f11aa (CI only, no hardware test). Kids edition is separate.
