#ifndef NYOTA_COLOR_H
#define NYOTA_COLOR_H

#include <stdint.h>

/*
 * NyotaUI color contract.
 *
 * 32 color families have DARK/base/LIGHT variants (96 names).
 * BLACK and WHITE are exact neutral endpoints and intentionally have no
 * DARK/LIGHT variants. TRANSPARENT and BACKDROP are special rendering modes.
 *
 * RGB values are language-level constants: hosts must not replace them with
 * platform theme colors. The same source must render the same colors on
 * AyoOS, Linux and Windows.
 */

#define NYOTA_COLOR_FAMILY_COUNT 32u
#define NYOTA_COLOR_NAMED_COUNT  98u
#define NYOTA_COLOR_SPECIAL_COUNT 2u

typedef enum {
    NYOTA_COLOR_SOLID = 0,
    NYOTA_COLOR_TRANSPARENT = 1,
    NYOTA_COLOR_BACKDROP = 2
} NyotaColorMode;

typedef struct {
    uint8_t mode;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} NyotaColor;

typedef struct {
    const char *name;
    uint32_t rgb;
} NyotaNamedColor;

static const NyotaNamedColor NYOTA_NAMED_COLORS[NYOTA_COLOR_NAMED_COUNT] = {
    {"BLACK", 0x000000u},
    {"WHITE", 0xFFFFFFu},
    {"DARKGRAY", 0x555555u},
    {"GRAY", 0x808080u},
    {"LIGHTGRAY", 0xB0B0B0u},
    {"DARKRED", 0x8B0000u},
    {"RED", 0xDC2626u},
    {"LIGHTRED", 0xFF6B6Bu},
    {"DARKRUBY", 0x7A0019u},
    {"RUBY", 0xC2183Au},
    {"LIGHTRUBY", 0xE85A76u},
    {"DARKMAROON", 0x4A0F1Bu},
    {"MAROON", 0x800020u},
    {"LIGHTMAROON", 0xB94A63u},
    {"DARKCRIMSON", 0x7A001Fu},
    {"CRIMSON", 0xDC143Cu},
    {"LIGHTCRIMSON", 0xF06A86u},
    {"DARKCORAL", 0xC24A3Au},
    {"CORAL", 0xFF6F61u},
    {"LIGHTCORAL", 0xFF9D94u},
    {"DARKSALMON", 0xC75C5Cu},
    {"SALMON", 0xFA8072u},
    {"LIGHTSALMON", 0xFFB0A5u},
    {"DARKORANGE", 0xB84E00u},
    {"ORANGE", 0xFF7A00u},
    {"LIGHTORANGE", 0xFFA34Du},
    {"DARKAMBER", 0xB36B00u},
    {"AMBER", 0xFFBF00u},
    {"LIGHTAMBER", 0xFFD666u},
    {"DARKGOLD", 0xA67C00u},
    {"GOLD", 0xD4AF37u},
    {"LIGHTGOLD", 0xF0D875u},
    {"DARKYELLOW", 0xB59B00u},
    {"YELLOW", 0xFFE600u},
    {"LIGHTYELLOW", 0xFFF46Au},
    {"DARKOLIVE", 0x5D6200u},
    {"OLIVE", 0x808000u},
    {"LIGHTOLIVE", 0xB3B34Du},
    {"DARKLIME", 0x4D9900u},
    {"LIME", 0x7CFC00u},
    {"LIGHTLIME", 0xB6FF66u},
    {"DARKGREEN", 0x006B2Du},
    {"GREEN", 0x00A846u},
    {"LIGHTGREEN", 0x5FD98Au},
    {"DARKEMERALD", 0x006644u},
    {"EMERALD", 0x009B77u},
    {"LIGHTEMERALD", 0x56C9A6u},
    {"DARKMINT", 0x4F9B83u},
    {"MINT", 0x98E2C6u},
    {"LIGHTMINT", 0xCDF5E6u},
    {"DARKTEAL", 0x005B5Bu},
    {"TEAL", 0x008080u},
    {"LIGHTTEAL", 0x55B7B7u},
    {"DARKTURQUOISE", 0x008A84u},
    {"TURQUOISE", 0x20B2AAu},
    {"LIGHTTURQUOISE", 0x6DDED8u},
    {"DARKCYAN", 0x0097A7u},
    {"CYAN", 0x00BCD4u},
    {"LIGHTCYAN", 0x67E8F9u},
    {"DARKSKY", 0x2C7FB8u},
    {"SKY", 0x56B4E9u},
    {"LIGHTSKY", 0xA7D9F5u},
    {"DARKAZURE", 0x005AA8u},
    {"AZURE", 0x007FFFu},
    {"LIGHTAZURE", 0x66B2FFu},
    {"DARKBLUE", 0x003A9Bu},
    {"BLUE", 0x0066FFu},
    {"LIGHTBLUE", 0x6EA8FFu},
    {"DARKSAPPHIRE", 0x082567u},
    {"SAPPHIRE", 0x0F52BAu},
    {"LIGHTSAPPHIRE", 0x5C8FE6u},
    {"DARKNAVY", 0x000040u},
    {"NAVY", 0x000080u},
    {"LIGHTNAVY", 0x4D4D9Du},
    {"DARKINDIGO", 0x2E1A66u},
    {"INDIGO", 0x4B0082u},
    {"LIGHTINDIGO", 0x8367A8u},
    {"DARKVIOLET", 0x5A189Au},
    {"VIOLET", 0x8F3FBFu},
    {"LIGHTVIOLET", 0xC58BE0u},
    {"DARKPURPLE", 0x4B146Du},
    {"PURPLE", 0x8000A8u},
    {"LIGHTPURPLE", 0xBA6AD0u},
    {"DARKMAGENTA", 0x9E0069u},
    {"MAGENTA", 0xD0008Fu},
    {"LIGHTMAGENTA", 0xF06BC2u},
    {"DARKPINK", 0xB52F69u},
    {"PINK", 0xFF69B4u},
    {"LIGHTPINK", 0xFFA6D2u},
    {"DARKBROWN", 0x5A2D0Cu},
    {"BROWN", 0x8B4513u},
    {"LIGHTBROWN", 0xC77A43u},
    {"DARKTAN", 0x8A623Du},
    {"TAN", 0xC19A6Bu},
    {"LIGHTTAN", 0xE0C09Bu},
    {"DARKBEIGE", 0xB8A98Au},
    {"BEIGE", 0xD9C8A9u},
    {"LIGHTBEIGE", 0xF3E8D0u},
};

static inline int NyotaColorNameEq(const char *a, const char *b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a == '\0' && *b == '\0';
}

static inline int NyotaColorHexDigit(char c, uint8_t *out) {
    if (c >= '0' && c <= '9') { *out = (uint8_t)(c - '0'); return 1; }
    if (c >= 'A' && c <= 'F') { *out = (uint8_t)(c - 'A' + 10); return 1; }
    if (c >= 'a' && c <= 'f') { *out = (uint8_t)(c - 'a' + 10); return 1; }
    return 0;
}

static inline int NyotaColorParseHexPair(const char *p, uint8_t *out) {
    uint8_t hi, lo;
    if (!NyotaColorHexDigit(p[0], &hi) || !NyotaColorHexDigit(p[1], &lo)) return 0;
    *out = (uint8_t)((hi << 4) | lo);
    return 1;
}

static inline int NyotaColorParse(const char *token, NyotaColor *out) {
    const char *p, *end;
    char name[32];
    uint32_t len = 0, i;

    if (!token || !out) return 0;
    p = token;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    end = p;
    while (*end) end++;
    while (end > p && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) end--;
    len = (uint32_t)(end - p);
    if (len == 0 || len >= sizeof(name)) return 0;

    if (len == 8 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        uint8_t r, g, b;
        if (!NyotaColorParseHexPair(p + 2, &r) ||
            !NyotaColorParseHexPair(p + 4, &g) ||
            !NyotaColorParseHexPair(p + 6, &b)) return 0;
        out->mode = NYOTA_COLOR_SOLID;
        out->r = r; out->g = g; out->b = b; out->a = 255;
        return 1;
    }

    if (len == 10 && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        uint8_t r, g, b, a;
        if (!NyotaColorParseHexPair(p + 2, &r) ||
            !NyotaColorParseHexPair(p + 4, &g) ||
            !NyotaColorParseHexPair(p + 6, &b) ||
            !NyotaColorParseHexPair(p + 8, &a)) return 0;
        out->mode = NYOTA_COLOR_SOLID;
        out->r = r; out->g = g; out->b = b; out->a = a;
        return 1;
    }

    for (i = 0; i < len; i++) name[i] = p[i];
    name[len] = '\0';

    if (NyotaColorNameEq(name, "TRANSPARENT")) {
        out->mode = NYOTA_COLOR_TRANSPARENT;
        out->r = 0; out->g = 0; out->b = 0; out->a = 0;
        return 1;
    }
    if (NyotaColorNameEq(name, "BACKDROP")) {
        out->mode = NYOTA_COLOR_BACKDROP;
        out->r = 0; out->g = 0; out->b = 0; out->a = 255;
        return 1;
    }

    for (i = 0; i < NYOTA_COLOR_NAMED_COUNT; i++) {
        if (NyotaColorNameEq(name, NYOTA_NAMED_COLORS[i].name)) {
            uint32_t rgb = NYOTA_NAMED_COLORS[i].rgb;
            out->mode = NYOTA_COLOR_SOLID;
            out->r = (uint8_t)((rgb >> 16) & 0xFFu);
            out->g = (uint8_t)((rgb >> 8) & 0xFFu);
            out->b = (uint8_t)(rgb & 0xFFu);
            out->a = 255;
            return 1;
        }
    }
    return 0;
}

static inline uint32_t NyotaColorRGBA(const NyotaColor *c) {
    if (!c) return 0;
    return ((uint32_t)c->r << 24) |
           ((uint32_t)c->g << 16) |
           ((uint32_t)c->b << 8) |
           (uint32_t)c->a;
}

#endif
