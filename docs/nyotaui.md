# NyotaUI — WIN i kontrolki

**Status:** WIN oraz pierwszy zestaw kontrolek NyotaUI mają składnię i backend POSIX/Linux.  
**Data decyzji:** 2026-09-18.  
**Zasada:** kod Nyoty jest identyczny na AyoOS, Linuxie i Windowsie; host realizuje ten sam kontrakt.

## 1. WIN

Każde okno ma rodzica. `ROOT` jest wbudowanym pseudo-rodzicem najwyższego poziomu i zawsze zapisuje się go wielkimi literami.

```nyota
WIN glowne, ROOT, [800, 600], CENTER, TRUE
WIN dialog, glowne, [400, 300], CENTER, FALSE
```

Ogólna postać:

```text
WIN nazwa, rodzic, rozmiar [, pozycja] [, resize]
```

Dozwolone warianty:

```nyota
WIN glowne, ROOT, [800, 600]
WIN glowne, ROOT, [800, 600], FALSE
WIN glowne, ROOT, [800, 600], [100, 80]
WIN glowne, ROOT, [800, 600], [100, 80], FALSE
WIN glowne, ROOT, [800, 600], CENTER
WIN glowne, ROOT, [800, 600], CENTER, FALSE
```

`resize` ma typ BOOLEAN i domyślnie wynosi `TRUE`.

## 2. Rozmiar i pozycja

Rozmiar jest wartością `LIST` z dokładnie dwoma elementami `INTEGER`: `[w, h]`. Obie wartości muszą być większe od zera. Pozycja, jeśli jest podana jawnie, także jest `LIST` z dokładnie dwoma `INTEGER`: `[x, y]`.

Nie jest wymagany literał listy. Każde wyrażenie zwracające poprawną listę jest dozwolone:

```nyota
VAR rozmiar := [800, 600]
VAR pozycja := [100, 80]
WIN glowne, ROOT, rozmiar, pozycja, FALSE
```

Wartości są odczytywane w chwili tworzenia okna. Późniejsza zmiana listy nie zmienia automatycznie geometrii istniejącego okna. Brak pozycji oznacza, że położenie wybiera host/system.

`CENTER` z `ROOT` centruje względem pulpitu/obszaru roboczego, a z rodzicem `WIN` względem tego rodzica. Jawne `[x, y]` jest liczone względem rodzica. Rodzic `WIN` opisuje logiczną hierarchię w stylu Tkintera; potomne `WIN` pozostaje osobnym oknem systemowym i nie jest clipowane do rodzica.

## 3. Opcjonalny CONFIG

```nyota
WIN glowne.CONFIG:
    TITLE = "Nyota"
    BG = DARKSAPPHIRE
    ICO = "nyota.png"
```

`CONFIG` jest opcjonalny i częściowy. Pierwsza wersja definiuje `TITLE` (STRING), `BG` (tło) i `ICO` (STRING ze ścieżką). Domyślny tytuł jest równy nazwie `WIN`, a domyślne tło to `BLACK`. `ICO` oznacza ikonę jako pojęcie, nie wymóg formatu `.ico`.

## 4. BG — kolor

`BG` może przyjąć nazwany kolor Nyoty, `0xRRGGBB`, `0xRRGGBBAA`, `TRANSPARENT` lub `BACKDROP`. Kolory są wartościami NyotaUI, więc mogą występować w LIST, np. `VAR kolory := [DARKBLUE, SAPPHIRE, SKY]`.

`BACKDROP` wymaga hosta zdolnego pobrać skomponowaną scenę pod obiektem. Backend POSIX/SDL2 obecnie jawnie odrzuca `BG = BACKDROP`, zamiast udawać to zachowanie.

## 5. BG — obraz

```nyota
BG = IMG("tlo.png")
BG = IMG("tlo.png", FIT)
BG = IMG("tlo.png", CROP)
BG = IMG("tlo.png", STRETCH)
BG = IMG("tlo.png", NATIVE)
BG = IMG("tlo.png", TILE)
```

`IMG(path)` oznacza domyślnie `CROP`. `FIT` zachowuje proporcje i pokazuje cały obraz, a niewypełniona przestrzeń jest `BLACK`. `CROP` zachowuje proporcje, wypełnia cały obszar i przycina nadmiar. `STRETCH` skaluje dokładnie do obszaru. `NATIVE` nie skaluje i zaczyna od lewego górnego rogu. `TILE` nie skaluje i powtarza obraz kafelkowo.

## 6. Gradient liniowy

```text
GRAD(LINEAR, kierunek, liczba_kolorow, lista_kolorow)
```

Kierunki: `VERTICAL`, `HORIZONTAL`, `DIAG_DOWN`, `DIAG_UP`.

```nyota
BG = GRAD(LINEAR, VERTICAL, 3, [BLACK, NAVY, SKY])
VAR kolory := [DARKBLUE, SAPPHIRE, SKY]
BG = GRAD(LINEAR, HORIZONTAL, 3, kolory)
```

Liczba kolorów musi odpowiadać długości LIST. Punkty kolorów są rozmieszczone równomiernie od 0% do 100%.

## 7. Gradient kształtowy

```text
GRAD(SHAPE, figura, centrum, kat, liczba_kolorow, lista_kolorow)
```

Dostępne figury pierwszej wersji: `CIRCLE`, `ELLIPSE`, `SQUARE`, `RECT`, `DIAMOND`, `STAR`, `EGG`. Centrum ma postać `CENTER` albo `[x, y]` i jest liczone względem własnego obszaru klienta obiektu posiadającego tło.

```nyota
BG = GRAD(SHAPE, CIRCLE, CENTER, 0, 3, [WHITE, BLUE, BLACK])
BG = GRAD(SHAPE, STAR, [220, 150], 45, 4, [WHITE, GOLD, ORANGE, DARKRED])
BG = GRAD(SHAPE, EGG, CENTER, 25, 3, [WHITE, GOLD, DARKRED])
```

Pierwszy kolor odpowiada centrum, kolejne są rozłożone równomiernie ku zewnętrzu. `kat` jest INTEGER w stopniach i obraca pole figury.

## 8. Gradient spiralny

```text
GRAD(SPIRAL, centrum, kat, obroty, kierunek, liczba_kolorow, lista_kolorow)
```

```nyota
BG = GRAD(SPIRAL, CENTER, 0, 3, CW, 3, [WHITE, BLUE, BLACK])
BG = GRAD(SPIRAL, [220, 150], 45, 5, CCW, 4, [WHITE, GOLD, ORANGE, DARKRED])
```

`obroty` są dodatnim INTEGER. `CW` oznacza zgodnie z ruchem wskazówek zegara, `CCW` przeciwnie. Centrum ma tę samą semantykę co w `SHAPE`.

## 9. Walidacja gradientów

`liczba_kolorow` musi mieścić się w zakresie 2..64 i dokładnie odpowiadać długości LIST. Każdy element listy musi być kolorem Nyoty. `BACKDROP` nie może być punktem gradientu; `TRANSPARENT` może nim być i wnosi alfę 0.

## 10. Wspólny model kontrolek

Kontrolki NyotaUI używają wspólnego konstruktora:

```text
KONTROLKA nazwa, rodzic, [w,h], [x,y]/CENTER [, WLASCIWOSC=wartosc ...]
```

Rodzicem może być `WIN` albo `PANEL`. `ROOT` jest rodzicem tylko dla `WIN`. Współrzędne kontrolki są liczone względem jej rodzica, a `CENTER` centruje ją w obszarze rodzica. Właściwości mogą być podane inline lub w bloku `.CONFIG:`.

### BUTTON

```nyota
BUTTON zapisz, glowne, [160, 48], [40, 40], TEXT="Zapisz"
BUTTON zapisz.CONFIG:
    FONT = "SYSTEM"
    FSIZE = 16
    CTEXT = WHITE
    BG = SAPPHIRE
```

`BUTTON_CLICKED(zapisz)` działa dla nowego BUTTON NyotaUI. Stara składnia BUTTON powiązana z GRAPH pozostaje tylko jako zgodność przejściowa i nie jest nową składnią NyotaUI.

### LABEL

```nyota
LABEL tytul, glowne, [320, 50], [20, 20], TEXT="Ustawienia"

LABEL tytul.CONFIG:
    FONT = "SYSTEM"
    FSIZE = 22
    CTEXT = WHITE
    BOLD = TRUE
    ITALIC = FALSE
    UNDERLINE = FALSE
    HALIGN = CENTER
    VALIGN = MIDDLE
    WRAP = FALSE
    BG = TRANSPARENT
    BORDER = TRUE
    CBORDER = GRAY
    BWIDTH = 1
```

`HALIGN`: `LEFT/CENTER/RIGHT`. `VALIGN`: `TOP/MIDDLE/BOTTOM`.

### PANEL

`PANEL` jest kontenerem. Dzieci liczą pozycję względem panelu, a panel może być zagnieżdżony w innym panelu.

```nyota
PANEL boczny, glowne, [280, 520], [20, 20],
      BG=GRAD(LINEAR, VERTICAL, 2, [DARKNAVY, BLACK]),
      BORDER=TRUE, CBORDER=GRAY, BWIDTH=1

PANEL boczny.CONFIG:
    CLIP = TRUE
```

`BG` panelu korzysta z dokładnie tego samego modelu co `WIN`: kolor, `IMG(...)` oraz `GRAD(...)`. PNG jest obowiązkowym formatem obrazu NyotaUI; backend może obsługiwać dodatkowe formaty, ale program nie powinien polegać na nich dla przenośności. Kanał alfa PNG jest zachowywany. `CLIP=TRUE` (domyślnie) przycina dzieci do obszaru panelu.

### DAREA — Drop Area

```nyota
DAREA import, glowne, [220, 220], [30, 80],
      TEXT="Upusc pliki",
      SHAPE=CIRCLE,
      ACCEPT=FILES,
      MULTI=TRUE,
      BORDER=TRUE,
      CBORDER=SAPPHIRE,
      BWIDTH=2

DAREA import.CONFIG:
    BG = DARKGRAY
    BGOVER = DARKSAPPHIRE
    CBORDEROVER = LIGHTSAPPHIRE
    BWIDTHOVER = 3
```

`SHAPE` ma dokładnie trzy wartości: `RECT`, `CIRCLE`, `ELLIPSE`. Domyślne jest `RECT`. Kształt definiuje zarówno rysowanie tła i ramki, jak i aktywny hit-test dropu. `CIRCLE` używa średnicy `min(w,h)`, natomiast `ELLIPSE` wypełnia zadany prostokąt.

`ACCEPT` przyjmuje `FILES`, `DIRS` albo `ALL`. `MULTI` jest BOOLEAN. Program odbiera zdarzenie i listę ścieżek przez:

```nyota
IF DAREA_DROPPED(import):
    VAR elementy := DAREA_ITEMS(import)
```

`DAREA_ITEMS()` zwraca LIST ścieżek przekazanych przez host.

## 11. Border

Wspólne właściwości ramki:

```text
BORDER   TRUE/FALSE
CBORDER  kolor
BWIDTH   INTEGER
```

Ramka jest rysowana wewnątrz zadeklarowanego rozmiaru kontrolki, więc `[300,40]` pozostaje rozmiarem `300x40` niezależnie od `BWIDTH`.

## 12. Hosty

Rdzeń przekazuje `WIN` przez `NyotaHost`; program `.nyo` nie używa SDL, AyoAPI ani WinAPI. Backend POSIX/Linux implementuje wiele okien SDL2, hierarchię rodziców i pozycjonowanie, resize, TITLE, ICO, obrazy przez SDL2_image, gradienty LINEAR/SHAPE/SPIRAL oraz BUTTON/LABEL/PANEL/DAREA. Kontrakt hosta nie zawiera typów SDL i pozostaje wspólny dla Linuxa, Windowsa i późniejszego backendu AyoOS.

Windows i AyoOS mają używać identycznego kodu `.nyo`; różnice należą wyłącznie do implementacji hosta. Backend Windows nie jest jeszcze zaimplementowany. AyoOS/Nexa/Sayari jest obecnie świadomie odłożony, ale kontrakt NyotaHost jest projektowany tak, aby nie wymagał późniejszej zmiany składni programu.
