/* nyota_host.h — backend hosta, nie wycinek języka.
 *
 * Nyota jest niezależnym językiem. Tunga jest osobnym edytorem.
 * AyoOS / Linux / Windows podłączają ten kontrakt.
 *
 * PRINT, GRAPH, INPUT, DELAY są poleceniami języka.
 * Host MUSI je zaimplementować. NULL nie jest poprawnym hostem desktop.
 */

#ifndef NYOTA_HOST_H
#define NYOTA_HOST_H

#include <stdint.h>

typedef struct NyotaHost {
    void (*emit)(char c);
    void (*emit_str)(const char *s);

    uint64_t (*unix_time)(void);
    uint64_t (*ticks_100hz)(void);

    /* INPUT: zwraca 1 i wypełnia buf, albo 0 przy błędzie. */
    int (*read_line)(char *buf, uint32_t max);

    void (*gfx_clear)(uint8_t r, uint8_t g, uint8_t b);
    void (*gfx_rect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t r, uint8_t g, uint8_t b);
    void (*gfx_mode)(int mode); /* GRAPH n */
} NyotaHost;

/* Wejście interpretera niezależne od edytora.
 * Tunga i przyszły VS Code wołają to samo. */
void NyotaEmbedRun(const char *src, void (*emit)(char c));

#endif
