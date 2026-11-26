# Neo Geo AES/MVS port — stage 1
The integrated Lynx, PCE HuCard and PCE CD (CUE/BIN) baseline at 8111c9b passed both workflows (45648398730 and 45648398686). No hardware testing yet.

Candidate decision: Geolith, BSD-3-Clause, pinned at https://github.com/libretro/geolith-libretro/tree/846c5b541b42c122105b2a1bb29830ba03e93058. FBNeo has a Neo Geo subset, but its non-commercial and donation restrictions require separate licensing consideration. Geolith is a focused C implementation with Musashi, Z80 and YMFM, and its direct memory APIs are promising for bare-metal use. Its cartridge format is .neo, not normal arcade ZIP sets; users may convert their own lawful dumps. Neo Geo CD is not requested and is not part of this stage.

CI first builds upstream on x86-64 Linux and ARMv7 soft-float Linux/QEMU, validates a conservative .neo cartridge layout (bounds, magic, length, overflow rejection), and attempts to run an original synthetic 68K/Z80 loop with synthetic BIOS ZIPs and save-state roundtrip. No actual BIOS or commercial ROM is included. Generated fixtures are temporary runner files, never released. The format validator intentionally rejects some unusual sizes until reviewed; it is not complete validation of protection metadata or BIOS contents.

Next: inspect actual results, use direct memory APIs and RGB565 conversion rather than Linux filesystem, design a separate large ROM arena (up to 128 MiB is a validator limit, not an approved device memory reservation), inventory bare-metal dependencies, validate native blob, add LK session/menu/SD persistence. No new scanner category or firmware packaging yet. Verify licensing notices of all bundled dependencies before distributing a core.

## Stage 2: native engine probe
Host and ARM Linux synthetic cartridge tests passed at 8ba3df2 (run 45653382484). The new bare-metal job compiles the upstream engine C translation units selected by the pinned Makefile with ARMv7 soft-float/newlib, combines them into a relocatable object and reports unresolved imports. Frontend and external dependency objects, final native linking, memory arena, RGB565 adapter and LK integration remain outstanding. This is not a runnable blob and must not be packaged.

## Stage 3: native ZIP and audio dependencies
Stage 2 passed at d555bda (runs 45653983122 and 45653983923). The native object now includes bundled miniz and Speex resampler sources and CI asserts their engine imports are resolved. CHD, filesystem, allocator, time and final linking remain open. No firmware integration yet.

## Stage 4: native compression dependency probe
Stage 3 passed at 138ae8c (runs 45659521590 and 45659521597). Include upstream libretro-common, LZMA and Zstandard objects selected by the pinned Makefile and build zlib for arm-none-eabi. Assert that CHD and major compression imports are resolved in the relocatable object. This does not enable Neo Geo CD or claim a runnable firmware core. Newlib portability and final linking remain under test.

## Stage 5: explicit native directory ABI
At ade32f41 the integration build passed (45668259114), and both runtime architectures passed, but native-engine failed (45668258926) because newlib has no dirent implementation. Provide a private declaration-only directory ABI for the relocatable native probe, without modifying upstream or pretending filesystem operations succeed. Require directory imports to remain visible in the inventory and reject implicit function declarations. A real bounded filesystem adapter and final link are still required; no firmware packaging is enabled. New CI results pending.

## Stage 6: directory entry type coverage
At 1caacd9 integration run 45682465913 and both runtime jobs passed; native-engine run 45682465862 failed compiling vfs_implementation.c because DT_LNK was undeclared. Add the conventional distinct link type value and a strict standalone ARM compiler probe of the directory ABI. This only declares a type; it does not enable symlink traversal or supply fake filesystem operations. Existing unresolved-import checks remain intact. Native build results pending; real filesystem adapter and final link still outstanding.

## Stage 7: native timestamp declaration
At fc1b8ee integration run 45700844197 and both runtime jobs passed; native-engine job 139510202410 in run 45700844178 failed on implicit utime declaration in miniz. Add a private POSIX-compatible declaration using time_t and compile-check its signature. Keep utime unresolved until a real platform adapter is linked. No timestamp writes or firmware packaging enabled. Native CI pending. Zlib configure also reported unsafe vsprintf fallback; resolve before final native linking/packaging.

## Stage 8: bounded native compression dependency
At bb82261 both workflows passed (45704158371 and 45704158379), including the relocatable ARM engine and both synthetic runtime jobs. The import inventory still contains platform file, allocation, time and libc functions. Build only the in-memory zlib translation units for native use, avoiding configure's unsafe vsprintf fallback and unused gzFile stdio. CI checks archive imports and required compression exports, and rejects unresolved compression or gz imports in the combined engine. No upstream changes, no firmware packaging, no hardware tests. New CI pending; platform adapter and final link remain open.

## Stage 9: repair workflow syntax
At 3c3357c integration run 45711228363 passed, but Geolith run 45711225813 failed before creating any jobs. The native zlib import regex was split by a duplicated workflow tail. Restore the anchored regex and single job tail, retaining the unsafe-import rejection and required-export checks. No native tests ran on that failed workflow; the previous green engine remains bb82261. New CI pending.

## Stage 10: bounded caller-owned heap primitive
At 917c199 both workflows passed (45719185001 and 45719184672); native compression checks passed. Native imports still include malloc/calloc/realloc/free, stdio, filesystem and time. Add a caller-owned bounded sbrk primitive with alignment and overflow checks, tested on host and ARM Linux and compiled for bare metal. No fixed device RAM reservation, no global newlib hook or firmware packaging yet. Existing PCE/Handy private heaps remain unchanged. Final allocator wiring, filesystem and native link remain open; new CI pending.

## Stage 11: private newlib break hook
At 298874d both workflows passed (45725954010 and 45725954206), including host and ARM Linux arena tests and native compilation. Bind the caller-owned arena once before allocations; reject rebinding even after the break returns to zero, preventing stale allocator state from being reused. Compile and link the private _sbrk hook into the relocatable native probe and verify its symbol. Host and ARM Linux tests exercise the same break logic without overriding host libc. No fixed RAM reservation or actual newlib malloc execution yet; final libc linking, filesystem, session lifecycle and firmware packaging remain open. New CI pending.

## Stage 12: real newlib allocator link probe
At ca2ec2e both workflows passed (45733998413 and 45733998421). Add a separate relocatable link probe using actual newlib calloc/realloc/free and the private _sbrk hook, asserting allocator imports resolve and required symbols exist. The probe includes zero-fill and preservation checks for future execution but is not executed in this stage. No device RAM reservation, no firmware packaging; remaining platform syscalls and filesystem integration are open. New CI pending.

## Stage 13: execute real newlib allocator
At 8bfe786 both workflows passed (45739932435 and 45739932430); native log explicitly confirmed link-only probe. Add a CI-only ARM Linux EABI exit harness to execute the actual bare-metal newlib allocator object under qemu-arm, without Linux libc or firmware changes. Extend probe with exhaustion, live-data preservation and reuse checks. Final executable linking must reject unresolved imports; QEMU exit status is mandatory. This is not a device test or a full engine link. Filesystem and device RAM layout remain open. New CI pending.

## Stage 14: bounded positional cartridge reader
At 59de688 both workflows passed (45752696882 and 45752696999); native log confirms real newlib allocator execution passed. Add a caller-owned positional reader for .neo cartridges, validate header and total length before payload reads, bound chunks to 32 KiB, reject short reads and insufficient destination capacity. Failed loads leave destination invalid; caller must not launch it. Host/ARM fixture tests and bare-metal compilation added, CI pending. Actual SD-FAT callback, BIOS ZIP I/O, saves and firmware integration remain open. No fixed RAM allocation or hardware test.

## SD cartridge bridge
At 9a49d11 both workflows passed (45759250346 and 45759250320), including native allocator execution. Add a read-only bridge from mounted FAT volume to bounded cartridge loader, with synthetic FAT16/32 image tests, invalid path/header and capacity checks. No scanner or firmware integration yet; FAT chain performance and BIOS/save access remain open. New CI pending.

## BIOS archive read bridge
At f0d73f3 both workflows passed (45766030096 and 45766030122, integration retry). Add bounded read-only BIOS archive loading from fixed /bios/aes.zip and /bios/neogeo.zip paths, with an 8 MiB provisional archive cap, ZIP signature screening and explicit failure length. FAT16/32 tests reuse original synthetic BIOS fixtures. This is not ZIP integrity or BIOS validation, nor engine integration; native archive consumption, saves and performance remain open. New CI pending.

## BIOS ZIP directory screening
At 23faa58 both workflows passed (45786924428, 45786924420); native newlib QEMU and FAT fixtures succeeded. Add conservative single-disk ZIP end-of-central-directory structure screening to the SD BIOS loader. The FAT16/32 fixture tests now also reject truncated archives and corrupted directory metadata, without including commercial BIOS data. This does NOT validate CRCs, members, BIOS content, or implement native archive extraction. Native engine filesystem integration, save persistence and RAM sizing remain open. CI for this stage pending.

## Complete classic ZIP directory bounds
At 975d4d9 both workflows passed (45796289624 and 45796289847), including ARM QEMU allocator execution and FAT16/FAT32 fixtures. Strengthen the BIOS archive screen to walk every central-directory entry, check local-header offsets, names, payload bounds, entry counts and reject encrypted, non-store/deflate and Zip64 sentinel records. Regression tests corrupt later entries, payload extents, encryption flags and entry counts. This is **structural validation only**: no decompression/CRC/authenticity validation, no engine BIOS binding, no save persistence, and no new firmware core. Native CI for this change is pending. Next: bounded in-memory ZIP member access and actual Geolith filesystem adapter without device allocation assumptions.

## Fix malformed ZIP fixture regression
At 74359dd the full integration build passed (45801375132), native-engine and ARM Linux probe passed, but the host runtime failed at sd_cart_test.c:56 (45801375076). The test changed only the lowest byte of the compressed-length field to 255; a small compressed member can still fit in the local payload region, so the parser correctly accepted it. Corrupt all four compressed-length bytes to 0xffffffff and restore all four afterwards, creating an unambiguously invalid length. The production parser remains unchanged; do not weaken validation or skip tests. New CI pending; no device testing or native engine BIOS integration yet.

## BIOS member span lookup
At a40cbb3 both workflows passed (45803722280 attempt 2 and 45803722312), including native allocator and FAT16/FAT32 tests. Add a bounded, read-only in-memory central-directory lookup returning a caller-owned member span, compression method, lengths and CRC metadata. Duplicate member names fail closed, missing names clear the output, and tests use only synthetic neo-po.bin archives. The API does not inflate or verify CRC/authenticity and does not claim BIOS execution; connecting the native decoder and engine, save-state persistence, RAM assessment and final device integration remain open. New CI results pending.

## CRC-checked native STORED member copy
At c6e9632 both workflows passed (45809121222 and 45809121194): FAT16/32 host fixtures, ARM Linux core probe, native allocator QEMU execution, and integration. Add a bounded, allocation-free CRC32-checked copy primitive for classic ZIP STORED members (method 0). Tests cover a known CRC vector, corruption, insufficient capacity, wrong method and length mismatch. ZIP DEFLATE (method 8), used by the synthetic BIOS archives, remains intentionally rejected by this copy primitive until a bounded native raw-inflate adapter is integrated. It does not claim a running native BIOS/core; final Geolith linkage, FAT persistence, menus and device memory validation are still open. New CI results pending.

## Raw DEFLATE member decoding contract
At 48d2183 both workflows passed (45812191594 and 45812191208), including host FAT fixtures, ARM Linux runtime, native newlib allocator QEMU and full integrated firmware build. Introduce a portable callback-based raw DEFLATE decoder with strict input consumption, exact output length and CRC32 verification, retaining the allocation-free STORED path. Host FAT16/32 fixtures exercise real raw-inflate via CI zlib against the synthetic neo-po.bin BIOS and reject wrong CRC, undersized destination, truncated data, absent inflater and dishonest byte counts. Native sd_cart remains freestanding and requires the final platform inflate adapter (no zlib symbol linked in this stage). Output buffer must be discarded on error. Next: bind vetted native zlib raw inflater with bounded RAM, then Geolith filesystem, saves and menu integration. CI pending; no flash/device tests.
