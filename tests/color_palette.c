#include <stdio.h>
#include "../src/nyota_color.h"

static int fail(const char *msg) {
    fprintf(stderr, "color_palette: %s\n", msg);
    return 1;
}

int main(void) {
    NyotaColor c;
    uint32_t i;

    if (NYOTA_COLOR_FAMILY_COUNT != 32u) return fail("family count");
    if (NYOTA_COLOR_NAMED_COUNT != 98u) return fail("named count");
    if (NYOTA_COLOR_SPECIAL_COUNT != 2u) return fail("special count");

    for (i = 0; i < NYOTA_COLOR_NAMED_COUNT; i++) {
        if (!NyotaColorParse(NYOTA_NAMED_COLORS[i].name, &c))
            return fail("named color does not parse");
        if (c.mode != NYOTA_COLOR_SOLID || c.a != 255)
            return fail("named color mode/alpha");
        if ((((uint32_t)c.r << 16) | ((uint32_t)c.g << 8) | c.b) != NYOTA_NAMED_COLORS[i].rgb)
            return fail("named color RGB mismatch");
    }

    if (!NyotaColorParse("RUBY", &c) || NyotaColorRGBA(&c) != 0xC2183AFFu)
        return fail("RUBY");
    if (!NyotaColorParse("DARKEMERALD", &c) || NyotaColorRGBA(&c) != 0x006644FFu)
        return fail("DARKEMERALD");
    if (!NyotaColorParse("LIGHTSAPPHIRE", &c) || NyotaColorRGBA(&c) != 0x5C8FE6FFu)
        return fail("LIGHTSAPPHIRE");

    if (!NyotaColorParse("TRANSPARENT", &c) ||
        c.mode != NYOTA_COLOR_TRANSPARENT || c.a != 0)
        return fail("TRANSPARENT");

    if (!NyotaColorParse("BACKDROP", &c) || c.mode != NYOTA_COLOR_BACKDROP)
        return fail("BACKDROP");

    if (!NyotaColorParse("0x123456", &c) || NyotaColorRGBA(&c) != 0x123456FFu)
        return fail("RRGGBB");
    if (!NyotaColorParse("0x12345680", &c) || NyotaColorRGBA(&c) != 0x12345680u)
        return fail("RRGGBBAA");
    if (!NyotaColorParse("  0Xabcdef40  ", &c) || NyotaColorRGBA(&c) != 0xABCDEF40u)
        return fail("hex case/whitespace");

    if (NyotaColorParse("DARKBLACK", &c)) return fail("DARKBLACK must not exist");
    if (NyotaColorParse("LIGHTWHITE", &c)) return fail("LIGHTWHITE must not exist");
    if (NyotaColorParse("#FF0000", &c)) return fail("#RRGGBB must not parse");
    if (NyotaColorParse("0x12345", &c)) return fail("short hex must not parse");

    puts("color_palette: PASS");
    return 0;
}
