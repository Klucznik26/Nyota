'use strict';

const FAMILY_ROWS = [
    ['GRAY', '555555', '808080', 'B0B0B0'],
    ['RED', '8B0000', 'DC2626', 'FF6B6B'],
    ['RUBY', '7A0019', 'C2183A', 'E85A76'],
    ['MAROON', '4A0F1B', '800020', 'B94A63'],
    ['CRIMSON', '7A001F', 'DC143C', 'F06A86'],
    ['CORAL', 'C24A3A', 'FF6F61', 'FF9D94'],
    ['SALMON', 'C75C5C', 'FA8072', 'FFB0A5'],
    ['ORANGE', 'B84E00', 'FF7A00', 'FFA34D'],
    ['AMBER', 'B36B00', 'FFBF00', 'FFD666'],
    ['GOLD', 'A67C00', 'D4AF37', 'F0D875'],
    ['YELLOW', 'B59B00', 'FFE600', 'FFF46A'],
    ['OLIVE', '5D6200', '808000', 'B3B34D'],
    ['LIME', '4D9900', '7CFC00', 'B6FF66'],
    ['GREEN', '006B2D', '00A846', '5FD98A'],
    ['EMERALD', '006644', '009B77', '56C9A6'],
    ['MINT', '4F9B83', '98E2C6', 'CDF5E6'],
    ['TEAL', '005B5B', '008080', '55B7B7'],
    ['TURQUOISE', '008A84', '20B2AA', '6DDED8'],
    ['CYAN', '0097A7', '00BCD4', '67E8F9'],
    ['SKY', '2C7FB8', '56B4E9', 'A7D9F5'],
    ['AZURE', '005AA8', '007FFF', '66B2FF'],
    ['BLUE', '003A9B', '0066FF', '6EA8FF'],
    ['SAPPHIRE', '082567', '0F52BA', '5C8FE6'],
    ['NAVY', '000040', '000080', '4D4D9D'],
    ['INDIGO', '2E1A66', '4B0082', '8367A8'],
    ['VIOLET', '5A189A', '8F3FBF', 'C58BE0'],
    ['PURPLE', '4B146D', '8000A8', 'BA6AD0'],
    ['MAGENTA', '9E0069', 'D0008F', 'F06BC2'],
    ['PINK', 'B52F69', 'FF69B4', 'FFA6D2'],
    ['BROWN', '5A2D0C', '8B4513', 'C77A43'],
    ['TAN', '8A623D', 'C19A6B', 'E0C09B'],
    ['BEIGE', 'B8A98A', 'D9C8A9', 'F3E8D0']
];

const namedColors = new Map([
    ['BLACK', '000000'],
    ['WHITE', 'FFFFFF']
]);

for (const [name, dark, base, light] of FAMILY_ROWS) {
    namedColors.set(`DARK${name}`, dark);
    namedColors.set(name, base);
    namedColors.set(`LIGHT${name}`, light);
}

const nameByRgb = new Map();
for (const [name, rgb] of namedColors) {
    if (!nameByRgb.has(rgb)) {
        nameByRgb.set(rgb, name);
    }
}

function byteFromHex(pair) {
    return Number.parseInt(pair, 16);
}

function parseNyotaColorToken(token) {
    const text = String(token || '').trim();

    if (text === 'BACKDROP') {
        return { mode: 'backdrop' };
    }

    if (text === 'TRANSPARENT') {
        return { mode: 'transparent', r: 0, g: 0, b: 0, a: 0 };
    }

    const named = namedColors.get(text);
    if (named) {
        return {
            mode: 'solid',
            r: byteFromHex(named.slice(0, 2)),
            g: byteFromHex(named.slice(2, 4)),
            b: byteFromHex(named.slice(4, 6)),
            a: 255
        };
    }

    const match = /^0[xX]([0-9A-Fa-f]{6})([0-9A-Fa-f]{2})?$/.exec(text);
    if (!match) {
        return null;
    }

    const rgb = match[1];
    return {
        mode: 'solid',
        r: byteFromHex(rgb.slice(0, 2)),
        g: byteFromHex(rgb.slice(2, 4)),
        b: byteFromHex(rgb.slice(4, 6)),
        a: match[2] ? byteFromHex(match[2]) : 255
    };
}

function hexByte(value) {
    return Math.max(0, Math.min(255, value))
        .toString(16)
        .toUpperCase()
        .padStart(2, '0');
}

function toNyotaHex(r, g, b, a = 255) {
    const rgb = `${hexByte(r)}${hexByte(g)}${hexByte(b)}`;
    return a === 255 ? `0x${rgb}` : `0x${rgb}${hexByte(a)}`;
}

function exactNamedColor(r, g, b, a = 255) {
    if (a !== 255) {
        return null;
    }
    return nameByRgb.get(`${hexByte(r)}${hexByte(g)}${hexByte(b)}`) || null;
}

module.exports = {
    FAMILY_ROWS,
    namedColors,
    parseNyotaColorToken,
    toNyotaHex,
    exactNamedColor
};
