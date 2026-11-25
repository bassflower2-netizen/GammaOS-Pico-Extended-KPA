# Neo Geo AES/MVS port — stage 1
The integrated Lynx, PCE HuCard and PCE CD (CUE/BIN) baseline at 8111c9b passed both workflows (45648398730 and 45648398686). No hardware testing yet.

Candidate decision: Geolith, BSD-3-Clause, pinned at https://github.com/libretro/geolith-libretro/tree/846c5b541b42c122105b2a1bb29830ba03e93058. FBNeo has a Neo Geo subset, but its non-commercial and donation restrictions require separate licensing consideration. Geolith is a focused C implementation with Musashi, Z80 and YMFM, and its direct memory APIs are promising for bare-metal use. Its cartridge format is .neo, not normal arcade ZIP sets; users may convert their own lawful dumps. Neo Geo CD is not requested and is not part of this stage.

CI first builds upstream on x86-64 Linux and ARMv7 soft-float Linux/QEMU, validates a conservative .neo cartridge layout (bounds, magic, length, overflow rejection), and attempts to run an original synthetic 68K/Z80 loop with synthetic BIOS ZIPs and save-state roundtrip. No actual BIOS or commercial ROM is included. Generated fixtures are temporary runner files, never released. The format validator intentionally rejects some unusual sizes until reviewed; it is not complete validation of protection metadata or BIOS contents.

Next: inspect actual results, use direct memory APIs and RGB565 conversion rather than Linux filesystem, design a separate large ROM arena (up to 128 MiB is a validator limit, not an approved device memory reservation), inventory bare-metal dependencies, validate native blob, add LK session/menu/SD persistence. No new scanner category or firmware packaging yet. Verify licensing notices of all bundled dependencies before distributing a core.

## Stage 2: native engine probe
Host and ARM Linux synthetic cartridge tests passed at 8ba3df2 (run 45653382484). The new bare-metal job compiles the upstream engine C translation units selected by the pinned Makefile with ARMv7 soft-float/newlib, combines them into a relocatable object and reports unresolved imports. Frontend and external dependency objects, final native linking, memory arena, RGB565 adapter and LK integration remain outstanding. This is not a runnable blob and must not be packaged.

## Stage 3: native ZIP and audio dependencies
Stage 2 passed at d555bda (runs 45653983122 and 45653983923). The native object now includes bundled miniz and Speex resampler sources and CI asserts their engine imports are resolved. CHD, filesystem, allocator, time and final linking remain open. No firmware integration yet.
