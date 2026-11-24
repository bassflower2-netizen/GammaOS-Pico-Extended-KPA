#ifndef LK_HANDY_CORE_LOADER_H
#define LK_HANDY_CORE_LOADER_H
#include "../gba/fat_ro.h"
#include "../../cores/handy-pico/handy_blob_abi.h"
/* Metadata check only. Safe to call without overwriting emulator DRAM. */
int handy_core_check(fat_vol *v);
/* Future session runner only: caller must own the armSNES/Genesis/NES overlay
 * exclusively and keep it until close() has returned. Never call from menu
 * diagnostics, background threads, or while another core is executing. */
const struct handy_blob_exports *handy_core_load(fat_vol *v, int *error);
#endif
