# PC Engine / TurboGrafx-16 — experimental native port

Pinned upstream: https://github.com/libretro/beetle-pce-fast-libretro/tree/4cf066e22533186d0cfad85a0fec2aced93e4e29 (GPL-2.0). The original notices stay in the upstream checkout. New adapter and test code in this directory use GPL-2.0-only.

First stage: a portable, memory-backed HuCard adapter for RGB565, 44.1 kHz audio, libretro joypad input, serialization and battery-backed RAM. The independent CI tests an original synthetic HuC6280 ROM on Linux x86-64 and Linux ARMv7 soft-float via QEMU, and attempts to compile the core objects with arm-none-eabi/newlib. The latter is only an object archive, not a fully linked bare-metal executable; it inventories unresolved dependencies for the next stage. No commercial ROM, BIOS or CD image is included.

The initial adapter accepts 8 KiB-aligned HuCards up to 2.5 MiB, with optional 512-byte copier header. The host build disables CHD and physical CD access, but retains bundled Tremor (required by the linked CD audio code even for HuCards) for the initial probe. PC Engine CD / Super CD-ROM requires separate file/sector access and BIOS tests; it is not implemented by this adapter.

Next: resolve native filesystem/libc dependencies, link and execute a flat ARM blob with the proven Lynx runtime pattern, add LK loader and session, then menu/ROM/save paths. Do not advertise PC Engine in the menu until the launch path is wired. Hardware testing is deferred by the project owner until all requested systems have been integrated.

Known successful Lynx integration baseline: 32fd3b48df82447370a04ac97617b67b928f11aa (CI only, no hardware test). Kids edition is separate.

Build correction: native newlib format definitions reuse the type-checked Handy header, and all ARM objects are combined into one relocatable object before unresolved-symbol inventory. This is still not a native runnable blob.

## Stage 2: flat ARM HuCard blob
The native object probe passed on 41a73db. This stage links the full core against newlib and a private 8 MiB heap with explicit file-access rejection. It tests the native ELF and then the exact flat image mapped at the existing 16 MiB emulator overlay (0x4F000000). The same synthetic ROM/save-state test runs through the export table. Linux syscalls are confined to the test executable. No LK loader, menu, SD persistence, or CD support yet. The output is experimental until CI and physical hardware verification.

## Stage 3: HuCard LK session
The successful flat test at d21289b is used as the basis for the new LK session. System ID 10 is reserved for PC Engine. The dedicated session owns the existing emulator overlay and sixth thread-stack slot. Its save paths are independent. Scanner and menu exposure follow separately. Hardware testing remains deferred. No rewind, cheats, achievements or FF in the initial driver.

## Stage 4: visible HuCard integration
System ID 10, .pce files, independent /saves/pce and /states/pce, Current/Classic artwork, and oem pce diagnostics are now wired. CD games are deliberately not scanned yet. Hardware verification is deferred. The integration build checks that the packaged core matches the exact QEMU-tested blob.

## Stage 5: CD stream probe
The complete HuCard integration at 9fa60e3 passed CI. A native, read-only file bridge now supports bounded virtual paths, positional file reads, a 4 KiB cache and optional System Card 3 BIOS (256 KiB, optional copier header). A dedicated synthetic fixture tests CUE/MODE1/2048 sector access with non-sequential reads, BIOS absence and path rejection, audio/video, BRAM state restoration and balanced handle closure. It runs the full ARM ELF via QEMU. CD entry is not exported by ABI 1; CD remains disabled in LK and not advertised by the scanner. The fixture BIOS is only a CPU loop, not a real firmware; no commercial content is included. CHD, actual SD FAT callbacks, mixed-mode CD audio and real BIOS/game testing remain outstanding.
