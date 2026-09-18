'use strict';

const fs = require('fs');
const path = require('path');
const {
    FAMILY_ROWS,
    namedColors,
    parseNyotaColorToken,
    toNyotaHex,
    exactNamedColor
} = require('./color-palette');

function assert(condition, message) {
    if (!condition) {
        throw new Error(message);
    }
}

assert(FAMILY_ROWS.length === 32, 'expected 32 color families');
assert(namedColors.size === 98, 'expected 98 ordinary named colors');

assert(parseNyotaColorToken('RUBY').r === 0xC2, 'RUBY parse');
assert(parseNyotaColorToken('DARKEMERALD').g === 0x66, 'DARKEMERALD parse');
assert(parseNyotaColorToken('LIGHTSAPPHIRE').b === 0xE6, 'LIGHTSAPPHIRE parse');
assert(parseNyotaColorToken('TRANSPARENT').mode === 'transparent', 'TRANSPARENT parse');
assert(parseNyotaColorToken('BACKDROP').mode === 'backdrop', 'BACKDROP parse');
assert(parseNyotaColorToken('DARKBLACK') === null, 'DARKBLACK must not exist');
assert(parseNyotaColorToken('LIGHTWHITE') === null, 'LIGHTWHITE must not exist');

const rgba = parseNyotaColorToken('0x12345680');
assert(rgba && rgba.r === 0x12 && rgba.g === 0x34 && rgba.b === 0x56 && rgba.a === 0x80,
    'RRGGBBAA parse');
assert(toNyotaHex(0x12, 0x34, 0x56, 0x80) === '0x12345680', 'RRGGBBAA format');
assert(exactNamedColor(0xC2, 0x18, 0x3A, 255) === 'RUBY', 'reverse named color');

const headerPath = path.resolve(__dirname, '../../src/nyota_color.h');
const header = fs.readFileSync(headerPath, 'utf8');
const cEntries = new Map();
for (const match of header.matchAll(/\{"([A-Z]+)", 0x([0-9A-F]{6})u\}/g)) {
    cEntries.set(match[1], match[2]);
}

assert(cEntries.size === namedColors.size, 'VS Code palette and C palette differ in size');
for (const [name, rgb] of namedColors) {
    assert(cEntries.get(name) === rgb, `palette mismatch for ${name}`);
}

console.log('vscode color palette: PASS');
