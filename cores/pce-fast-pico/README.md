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

## Stage 6: ABI 2 native CD export
Baseline 77478fd passed both workflows: native synthetic CD test reported 8 video callbacks, 5871 audio frames, 70 reads including 4 track reads, 6 balanced opens/closes, and 415222-byte state. ABI 2 adds the CD entry to the existing export table. LK and SD packaging require ABI 2 together; do not mix ABI 1 cards with this bootloader. CI exercises the exact packaged blob for CD open, BIOS absence, path rejection, state save/restore, close/reopen, and subsequent HuCard launch. The direct non-sequential sector test remains in the native ELF and is mandatory in integration CI. CD category remains disabled until SD FAT callbacks and scanner/session dispatch are implemented. Hardware, real BIOS, mixed-mode audio and CHD are unverified.

## Stage 7: SD FAT bridge
ABI 2 passed native and full integration CI at 717ad11 (runs 45620639033 and 45620638942). New LK-side read-only callbacks map the virtual disc directory and BIOS to the existing FAT reader. CI tests FAT16/32 images, random reads, failed I/O, path containment, handle capacity and cleanup. This bridge is compiled but not exposed to the game scanner/session yet. It uses fat_read, which rewalks FAT chains for random offsets; large fragmented discs need performance verification and possibly a cursor/index optimization. exFAT is routed through the existing backend but is not covered by these new fixture tests. Next: CD session integration, separate library/category and saves, CUE validation and real BIOS/game verification later. CHD remains pending.

## Stage 8: CD session and library
FAT16/32 bridge tests passed at 49b6432 (integration run 45642550495). System ID 11 adds a separate CD category, ROM scanner for unarchived CUE descriptors, separate controls, BRAM and states, and shared PCE worker/ABI 2. The full filename is recovered by directory-entry identity rather than the truncated library label. CD handles are released after core close and on failure. Current theme distinguishes CD by text logo; Classic reuses the PCE console logo initially. Track paths are limited to the same folder. CD audio, exFAT, large fragmented images and real BIOS/game behavior remain unverified. CHD is not enabled. No physical testing until all cores are integrated.
