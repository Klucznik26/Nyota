/* nyota_host.h — backend hosta, nie wycinek języka.
 *
 * Nyota jest niezależnym językiem. Tunga jest osobnym edytorem.
 * AyoOS / Linux / Windows podłączają ten kontrakt.
 *
 * PRINT, GRAPH, INPUT, DELAY są poleceniami języka.
 */

#ifndef NYOTA_HOST_H
#define NYOTA_HOST_H

#include <stdint.h>

typedef struct NyotaHost {
    void (*emit)(char c);
    void (*emit_str)(const char *s);

    uint64_t (*unix_time)(void);
    uint32_t (*local_time_seconds)(void);
    uint64_t (*ticks_100hz)(void);

    uint8_t (*wait_key)(void);
    uint8_t (*key_mods)(void);
    /* Zwraca maskę przycisków wskaźnika: bit 0 = lewy. */
    uint8_t (*pointer_state)(int32_t *x, int32_t *y);

    void (*gfx_clear)(uint8_t r, uint8_t g, uint8_t b);
    void (*gfx_rect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t r, uint8_t g, uint8_t b);
    void (*gfx_text)(uint32_t x, uint32_t y, const char *text,
                     uint8_t r, uint8_t g, uint8_t b, uint32_t scale);
    void (*gfx_mode)(uint32_t w, uint32_t h);

    /* Sprite backend. Language-level SPRITE is named state in Nyota;
     * host owns only the loaded image resource and drawing. */
    int32_t (*gfx_sprite_load)(const char *path);
    void (*gfx_sprite_free)(int32_t handle);
    void (*gfx_sprite_draw)(int32_t handle, int32_t x, int32_t y,
                            uint32_t w, uint32_t h);
    void (*gfx_sprite_draw_frame)(int32_t handle, uint32_t frame,
                                  uint32_t frame_count,
                                  int32_t x, int32_t y,
                                  uint32_t w, uint32_t h);
} NyotaHost;

void NyotaSetHost(NyotaHost *h);
void NyotaEmbedRun(const char *src, void (*emit)(char c));

#endif
