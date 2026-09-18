# NyotaUI — kolory

**Status:** kontrakt palety i parser wdrożone; integracja z właściwościami kontrolek NyotaUI będzie wykonywana razem z ich składnią.  
**Data decyzji:** 2026-09-18

## Zasada

Paleta jest częścią Nyoty, a nie motywu systemu. Ta sama nazwa koloru ma tę
samą wartość na AyoOS, Linuxie i Windowsie.

Nyota definiuje **32 rodziny kolorów**, każdą w trzech wariantach:
`DARK`, podstawowym i `LIGHT`. Daje to 96 kolorów rodzinnych.

`BLACK` i `WHITE` są osobnymi, dokładnymi punktami krańcowymi:

```text
BLACK = 0x000000
WHITE = 0xFFFFFF
```

Nie istnieją `DARKBLACK`, `LIGHTBLACK`, `DARKWHITE` ani `LIGHTWHITE`.
Odcienie między czernią i bielą zapewnia rodzina `GRAY`.

Łącznie istnieje **98 zwykłych nazwanych kolorów**.

## 32 rodziny

| Rodzina | DARK | podstawowy | LIGHT |
|---|---|---|---|
| GRAY | `DARKGRAY` `0x555555` | `GRAY` `0x808080` | `LIGHTGRAY` `0xB0B0B0` |
| RED | `DARKRED` `0x8B0000` | `RED` `0xDC2626` | `LIGHTRED` `0xFF6B6B` |
| RUBY | `DARKRUBY` `0x7A0019` | `RUBY` `0xC2183A` | `LIGHTRUBY` `0xE85A76` |
| MAROON | `DARKMAROON` `0x4A0F1B` | `MAROON` `0x800020` | `LIGHTMAROON` `0xB94A63` |
| CRIMSON | `DARKCRIMSON` `0x7A001F` | `CRIMSON` `0xDC143C` | `LIGHTCRIMSON` `0xF06A86` |
| CORAL | `DARKCORAL` `0xC24A3A` | `CORAL` `0xFF6F61` | `LIGHTCORAL` `0xFF9D94` |
| SALMON | `DARKSALMON` `0xC75C5C` | `SALMON` `0xFA8072` | `LIGHTSALMON` `0xFFB0A5` |
| ORANGE | `DARKORANGE` `0xB84E00` | `ORANGE` `0xFF7A00` | `LIGHTORANGE` `0xFFA34D` |
| AMBER | `DARKAMBER` `0xB36B00` | `AMBER` `0xFFBF00` | `LIGHTAMBER` `0xFFD666` |
| GOLD | `DARKGOLD` `0xA67C00` | `GOLD` `0xD4AF37` | `LIGHTGOLD` `0xF0D875` |
| YELLOW | `DARKYELLOW` `0xB59B00` | `YELLOW` `0xFFE600` | `LIGHTYELLOW` `0xFFF46A` |
| OLIVE | `DARKOLIVE` `0x5D6200` | `OLIVE` `0x808000` | `LIGHTOLIVE` `0xB3B34D` |
| LIME | `DARKLIME` `0x4D9900` | `LIME` `0x7CFC00` | `LIGHTLIME` `0xB6FF66` |
| GREEN | `DARKGREEN` `0x006B2D` | `GREEN` `0x00A846` | `LIGHTGREEN` `0x5FD98A` |
| EMERALD | `DARKEMERALD` `0x006644` | `EMERALD` `0x009B77` | `LIGHTEMERALD` `0x56C9A6` |
| MINT | `DARKMINT` `0x4F9B83` | `MINT` `0x98E2C6` | `LIGHTMINT` `0xCDF5E6` |
| TEAL | `DARKTEAL` `0x005B5B` | `TEAL` `0x008080` | `LIGHTTEAL` `0x55B7B7` |
| TURQUOISE | `DARKTURQUOISE` `0x008A84` | `TURQUOISE` `0x20B2AA` | `LIGHTTURQUOISE` `0x6DDED8` |
| CYAN | `DARKCYAN` `0x0097A7` | `CYAN` `0x00BCD4` | `LIGHTCYAN` `0x67E8F9` |
| SKY | `DARKSKY` `0x2C7FB8` | `SKY` `0x56B4E9` | `LIGHTSKY` `0xA7D9F5` |
| AZURE | `DARKAZURE` `0x005AA8` | `AZURE` `0x007FFF` | `LIGHTAZURE` `0x66B2FF` |
| BLUE | `DARKBLUE` `0x003A9B` | `BLUE` `0x0066FF` | `LIGHTBLUE` `0x6EA8FF` |
| SAPPHIRE | `DARKSAPPHIRE` `0x082567` | `SAPPHIRE` `0x0F52BA` | `LIGHTSAPPHIRE` `0x5C8FE6` |
| NAVY | `DARKNAVY` `0x000040` | `NAVY` `0x000080` | `LIGHTNAVY` `0x4D4D9D` |
| INDIGO | `DARKINDIGO` `0x2E1A66` | `INDIGO` `0x4B0082` | `LIGHTINDIGO` `0x8367A8` |
| VIOLET | `DARKVIOLET` `0x5A189A` | `VIOLET` `0x8F3FBF` | `LIGHTVIOLET` `0xC58BE0` |
| PURPLE | `DARKPURPLE` `0x4B146D` | `PURPLE` `0x8000A8` | `LIGHTPURPLE` `0xBA6AD0` |
| MAGENTA | `DARKMAGENTA` `0x9E0069` | `MAGENTA` `0xD0008F` | `LIGHTMAGENTA` `0xF06BC2` |
| PINK | `DARKPINK` `0xB52F69` | `PINK` `0xFF69B4` | `LIGHTPINK` `0xFFA6D2` |
| BROWN | `DARKBROWN` `0x5A2D0C` | `BROWN` `0x8B4513` | `LIGHTBROWN` `0xC77A43` |
| TAN | `DARKTAN` `0x8A623D` | `TAN` `0xC19A6B` | `LIGHTTAN` `0xE0C09B` |
| BEIGE | `DARKBEIGE` `0xB8A98A` | `BEIGE` `0xD9C8A9` | `LIGHTBEIGE` `0xF3E8D0` |

## Dwa tryby specjalne

### TRANSPARENT

`TRANSPARENT` nie usuwa obiektu i nie oznacza `VISIBLE = FALSE`.

Dana warstwa jest nadal częścią renderowanego obiektu, zachowuje geometrię,
stan i zdarzenia, lecz ma zerową alfę. Dzięki temu np. tekst przycisku może być
niewidoczny w stanie podstawowym i otrzymać kolor po `HOVER`.

### BACKDROP

`BACKDROP` oznacza pobranie koloru piksel po pikselu ze skomponowanej sceny
znajdującej się **pod obiektem posiadającym daną warstwę**.

Przykład przyszłej konfiguracji:

```nyota
button.CONFIG:
    BG = GREEN
    CTEXT = BACKDROP
```

Jeżeli tekst przycisku leży częściowo nad niebieskim tłem, a częściowo nad
innym oknem, poszczególne piksele tekstu mogą mieć różne kolory zgodne z
rzeczywistym obrazem znajdującym się pod przyciskiem.

`BACKDROP` nie oznacza koloru rodzica i nie oznacza przezroczystości.
Backend hosta musi użyć obrazu sceny sprzed narysowania bieżącego obiektu.

## Zapis szesnastkowy

Oprócz nazw palety parser kolorów przyjmuje:

```text
0xRRGGBB
0xRRGGBBAA
```

Przykłady:

```nyota
BG = 0x18344A
CTEXT = 0xF0C020
BG = 0x20202080
```

`#RRGGBB` nie jest składnią koloru, ponieważ znak `#` rozpoczyna komentarz
w Nyocie.

## Liczby kontraktu

```text
32 rodziny × 3 warianty = 96
BLACK + WHITE            = 2
--------------------------------
zwykłe nazwane kolory    = 98

TRANSPARENT + BACKDROP   = 2 tryby specjalne
--------------------------------
symboliczne wartości     = 100
```

Implementacja parsera znajduje się w `src/nyota_color.h`.
