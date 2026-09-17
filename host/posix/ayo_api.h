/* Minimalny AyoAPI dla hosta POSIX. Tylko to, czego używa interpreter Nyoty.
 * Nie jest to pełne AyoOS API. */
#ifndef AYO_API_H
#define AYO_API_H

#include <stdint.h>

typedef struct AyoAPI {
    void (*SetResolution)(uint32_t w, uint32_t h);
    void (*ClearScreen)(uint8_t r, uint8_t g, uint8_t b);
    void (*DrawRect)(uint32_t x, uint32_t y, uint32_t w, uint32_t h,
                     uint8_t r, uint8_t g, uint8_t b);
    void (*DrawText)(uint32_t x, uint32_t y, const char *text,
                     uint8_t r, uint8_t g, uint8_t b, uint32_t scale);
    uint8_t (*WaitForKey)(void);
    uint8_t (*GetKeyModifiers)(void);
    int (*ReadFile)(const char *path, uint8_t *buffer, uint32_t buffer_size,
                    uint32_t *out_size);
    uint64_t (*GetTicks)(void);
    void (*Exit)(void);
    uint64_t (*GetUnixTime)(void);
} AyoAPI;

#endif
