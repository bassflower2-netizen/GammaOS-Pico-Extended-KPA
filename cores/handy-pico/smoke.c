#include "handy_adapter.h"
#include "libretro.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned videos, audio_frames;
static void video(const uint16_t *data, unsigned w, unsigned h, size_t pitch)
{
    assert(w && h && w <= 160 && h <= 160);
    if (data) assert(pitch >= w * 2);
    ++videos;
}
static size_t audio(const int16_t *data, size_t n)
{
    if (n) assert(data);
    audio_frames += n;
    return n;
}
int main(void)
{
    /* Original, synthetic 65C02 homebrew: BRA past BS93 header, JMP to self.
     * Loaded at $2000; no copyrighted game code or BIOS. */
    static const uint8_t rom[] = {
        0x80,0x08,0x20,0x0a,0x00,0x0d,'B','S','9','3',0x4c,0x0a,0x20
    };
    struct handy_pico_callbacks cb = {video, audio};
    assert(handy_pico_frame(0) < 0);
    assert(handy_pico_open(rom, 5, &cb) < 0);
    assert(handy_pico_open(rom, sizeof(rom), &cb) == 0);
    assert(handy_pico_ram_size() == 65536);
    uint8_t *ram = handy_pico_ram();
    assert(ram && memcmp(ram + 0x2000, rom, sizeof(rom)) == 0);
    for (unsigned i = 0; i < 8; ++i)
        assert(handy_pico_frame(i & 1 ? 1u << RETRO_DEVICE_ID_JOYPAD_A : 0) == 0);
    assert(videos >= 8 && audio_frames > 0);
    handy_pico_reset();
    assert(handy_pico_frame(0) == 0);
    size_t size = handy_pico_state_size();
    assert(size > 65536 && size < 16 * 1024 * 1024);
    uint8_t *state = malloc(size);
    assert(state);
    ram[0x1000] = 0x42;
    assert(handy_pico_save(state, size) == 0);
    ram[0x1000] = 0x99;
    assert(handy_pico_load(state, size) == 0);
    assert(ram[0x1000] == 0x42);
    free(state);
    handy_pico_close();
    assert(!handy_pico_ram() && !handy_pico_state_size());
    printf("Handy adapter smoke: %u video callbacks, %u stereo samples, %zu byte state\n",
           videos, audio_frames, size);
    return 0;
}
