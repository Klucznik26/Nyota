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


---

## CBOX

`CBOX` jest samodzielnym polem wyboru bez wbudowanego opisu. Opis tworzy się osobnym `LABEL`.

```nyota
CBOX zgoda, glowne, [20, 20]
CBOX zgoda2, glowne, [20, 60], 2
```

Ostatni parametr to logiczny rozmiar kontrolki; domyślnie `1`. Kolejne wartości `2`, `3`, ... skalują pole. W aktualnym backendzie POSIX jedna jednostka odpowiada bazowemu polu 20x20 jednostek NyotaUI.

```nyota
CBOX zgoda.CONFIG:
    BG = WHITE
    BORDER = TRUE
    CBORDER = GRAY
    BWIDTH = 1
    CCHECK = EMERALD
    CHECKED = TRUE
    CSYMBOL = X
```

`CSYMBOL=X` jest domyślny. Można podać inny niepusty symbol jako `STRING`, np. `CSYMBOL="+"`. Aktualny renderer POSIX używa prostego fontu 8x8 i nie gwarantuje jeszcze pełnego Unicode.

Stan:
```nyota
CBOX_CHECKED(zgoda)
CBOX_SET(zgoda, TRUE)
```

## RADIO

`RADIO` jest zawsze kołem. Składnia rozmiaru i wyglądu jest zgodna z `CBOX`, ale kontrolki łączy się przez `RGROUP`.

```nyota
RADIO r1, glowne, [20, 20], 1
RADIO r2, glowne, [20, 60], 1

RADIO r1.CONFIG:
    RGROUP = tryb
    CHECKED = TRUE
    BG = WHITE
    BORDER = TRUE
    CBORDER = GRAY
    BWIDTH = 1
    CCHECK = SAPPHIRE

RADIO r2.CONFIG:
    RGROUP = tryb
```

W jednej grupie zaznaczenie jednego `RADIO` odznacza pozostałe. Odczyt i ustawienie stanu: `RADIO_CHECKED(...)`, `RADIO_SET(..., BOOLEAN)`.

## COMBO

```nyota
VAR tryby := ["Automatyczny", "Reczny", "Wylaczony"]

COMBO tryb, glowne, [220, 32], [20, 20],
      ITEMS=tryby,
      SELECTED=0
```

`.CONFIG`:

```nyota
COMBO tryb.CONFIG:
    BG = DARKGRAY
    CTEXT = WHITE
    BORDER = TRUE
    CBORDER = GRAY
    BWIDTH = 1
    CARROW = WHITE

    BGDROP = BLACK
    CDROP = LIGHTGRAY
    BGSELECT = SAPPHIRE
    CSELECT = WHITE

    HOVER = ON
    BGHOVER = DARKSAPPHIRE
    CHOVER = WHITE
    MAXVISIBLE = 8
    RADIUS = 6
```

`HOVER` przyjmuje `ON/OFF`. Odczyt: `COMBO_INDEX(...)`, `COMBO_VALUE(...)`; ustawienie indeksu: `COMBO_SET(..., indeks)`.

## SEP

```nyota
SEP linia, panel, [20, 120], 260
SEP pion, panel, [300, 20], 180, VERTICAL
```

Domyślna orientacja to `HORIZONTAL`.

```nyota
SEP linia.CONFIG:
    EFFECT = INSET
    CSEP = GRAY
    THICK = 2
```

`EFFECT`: `NORMAL`, `INSET`, `RAISED`, `GRADIENT`. Dla `INSET/RAISED` wymagane jest `THICK >= 2`. Przy `GRADIENT` używa się zwykłego `BG = GRAD(...)`.

## SBAR

`SBAR` jest interaktywnym paskiem przewijania / kontrolką zakresu.

```nyota
SBAR poziom, panel, [320, 28], [20, 20], MIN=0, MAX=100, VALUE=35, PAGE=20, STEP=5
```

Najważniejsze właściwości:

- `MIN`, `MAX`, `VALUE` — zakres i bieżąca wartość,
- `PAGE` — rozmiar strony wpływający na długość przesuwaka,
- `STEP` — krok klawiatury,
- `ORIENTATION=HORIZONTAL/VERTICAL`,
- `CTHUMB` i `CTHUMBOVER` — kolor przesuwaka,
- `THUMB` — geometria przesuwaka.

Obsługiwane figury przesuwaka:

```text
RECT
ROUND
CIRCLE
DIAMOND
TRIANGLE
PARALLELOGRAM
```

Przykład:

```nyota
SBAR pozycja, panel, [360, 30], [20, 60],
     MIN=0, MAX=1000, VALUE=250, PAGE=100, STEP=10,
     THUMB=PARALLELOGRAM,
     CTHUMB=SAPPHIRE,
     CTHUMBOVER=LIGHTSAPPHIRE
```

Na hoście POSIX `SBAR` obsługuje kliknięcie toru, przeciąganie przesuwaka, focus klawiatury, strzałki, `PageUp/PageDown`, `Home` i `End`.

Odczyt i ustawienie wartości:

```nyota
VAR x := SBAR_VALUE(pozycja)
VAR ok := SBAR_SET(pozycja, 500)
```

## PBAR

`PBAR` jest paskiem postępu. Wspiera klasyczny ciągły pasek oraz postęp zbudowany z powtarzanych figur.

```nyota
PBAR postep, panel, [420, 32], [20, 120],
     MIN=0, MAX=100, VALUE=65,
     SHAPE=CIRCLE,
     SEGMENTS=20,
     SPACING=5,
     CFILL=EMERALD,
     CEMPTY=DARKGRAY
```

`SHAPE` może mieć wartość:

```text
BAR
CIRCLE
TRIANGLE
SQUARE
PARALLELOGRAM
```

Dla `BAR` postęp jest ciągły. Dla pozostałych figur `SEGMENTS` określa liczbę elementów, a `SPACING` odstęp między nimi. `CFILL` określa kolor części wykonanej, a `CEMPTY` kolor elementów jeszcze niewypełnionych.

`ORIENTATION=HORIZONTAL/VERTICAL` działa zarówno dla zwykłego paska, jak i wersji segmentowej. Pionowy `PBAR` wypełnia się od dołu ku górze.

Odczyt i ustawienie wartości:

```nyota
VAR p := PBAR_VALUE(postep)
VAR ok := PBAR_SET(postep, 80)
```

## SHADOW dziedziczony z WIN

Cień definiuje się wyłącznie w `WIN.CONFIG`; wszystkie kontrolki należące do tego okna dziedziczą tę samą politykę cienia. Nie jest to cień dekoracji systemowego okna `WIN ... ROOT`.

```nyota
WIN glowne.CONFIG:
    SHADOW = RD
    CSHADOW = BLACK
    SDEPTH = 2
```

`SHADOW`: `OFF`, `R`, `L`, `U`, `D`, `RU`, `RD`, `LU`, `LD`.

## RADIUS

`RADIUS` zaokrągla prostokątne tło i border. Działa m.in. dla `PANEL`, `BUTTON`, `COMBO`, `TAREA`, `SBAR`, `PBAR`, prostokątnego `DAREA` i obszaru zawartości `TAB`. `DAREA` z `CIRCLE/ELLIPSE` nie przyjmuje `RADIUS`.

## TABS / TAB

`TABS` jest kontenerem zakładek, a każda `TAB` jest pełnoprawnym rodzicem dla kontrolek.

```nyota
TABS ustawienia, glowne, [500, 320], [20, 20]

TAB general, ustawienia, "General"
TAB driver, ustawienia, "Driver"
TAB resources, ustawienia, "Resources"

LABEL opis, general, [220, 24], [20, 20], TEXT="Ustawienia ogolne"
```

Zawartość `TAB` ma ten sam model co `PANEL`: `BG`, `BORDER`, `CBORDER`, `BWIDTH`, `CLIP`, `RADIUS`.

Główka zakładki ma osobne właściwości:

```nyota
TAB general.CONFIG:
    TABBG = SAPPHIRE
    TABFONT = "SYSTEM"
    TABFSIZE = 14
    TABCTEXT = WHITE
    TABBOLD = TRUE
    TABITALIC = FALSE
    TABUNDERLINE = FALSE
    TABBORDER = TRUE
    TABCBORDER = LIGHTGRAY
    TABBWIDTH = 1
    TABRADIUS = 6

    BG = DARKGRAY
    BORDER = TRUE
    CBORDER = GRAY
    BWIDTH = 1
    RADIUS = 8
    CLIP = TRUE
```

`TABBORDER` rysuje górną oraz boczne krawędzie główki, bez dolnej. `TABRADIUS` zaokrągla górne rogi. Dzieci `TAB` używają współrzędnych względem jej obszaru zawartości.


## Warstwa wizualna POSIX — modernizacja

Backend POSIX NyotaUI używa teraz SDL2_ttf i fontconfig do antyaliasowanego tekstu UTF-8. `FONT="SYSTEM"` wybiera normalny font systemowy hosta; nazwana rodzina w `FONT` jest rozwiązywana przez fontconfig. Bitmapowy font 8x8 pozostaje wyłącznie awaryjnym fallbackiem.

Kontrolki interaktywne mają hostowe stany wizualne hover/pressed bez zmiany kodu programu Nyota. Dotyczy to obecnie `BUTTON`, `CBOX`, `RADIO`, `COMBO` i główek `TAB`. `DAREA` nadal respektuje jawne właściwości `BGOVER`, `CBORDEROVER` i `BWIDTHOVER`.

Domyślny motyw NyotaUI jest neutralnym ciemnym motywem desktopowym. Jawne ustawienia w `.CONFIG` nadal mają pierwszeństwo.

Dla `TAB` właściwość `RADIUS` dotyczy części zawartości i zaokrągla tylko dwa dolne rogi. Górna krawędź pozostaje prosta, aby wizualnie łączyć zawartość z główką zakładki. `TABRADIUS` dotyczy główki i zaokrągla jej górne rogi.


## Stany interaktywne

Kontrolki interaktywne `BUTTON`, `CBOX`, `RADIO`, `COMBO`, `DAREA`, `TAB` i `SBAR` mają stan `ENABLED`.

```nyota
BUTTON zapisz, panel, [120, 36], [20, 20], TEXT="Zapisz", ENABLED=FALSE
```

`ENABLED=FALSE` wyszarza kontrolkę i wyłącza obsługę wejścia. Domyślna wartość to `TRUE`.

Backend POSIX utrzymuje też focus klawiatury dla kontrolek interaktywnych. Kliknięcie nadaje focus, a `Tab` oraz `Shift+Tab` przechodzą między aktywnymi kontrolkami. `Enter`/`Space` aktywują kontrolkę z focusem: `BUTTON`, `CBOX`, `RADIO`, `COMBO` lub `TAB`. Focus jest sygnalizowany subtelnym obrysem i nie wymaga dodatkowej właściwości w kodzie Nyoty.


## Wygładzanie geometrii POSIX

Zaokrąglone tła, kontrolki kołowe oraz eliptyczne otrzymują na hoście POSIX antyaliasowaną maskę krawędzi. Dotyczy to między innymi `RADIUS`, `RADIO`, `DAREA CIRCLE/ELLIPSE` i obu części `TAB`.

Cienie zachowują teraz geometrię kontrolki zamiast być zawsze prostokątem. Nadal obowiązuje kontrakt bez rozmycia; `SHADOW` pozostaje cieniem kierunkowym o określonej głębokości.


Zaokrąglone i kołowe bordery oraz obrysy focusu są na hoście POSIX renderowane z antyaliasingiem. Focus na `TAB` jest zaznaczany delikatnym akcentem na główce, bez dodawania dolnej krawędzi oddzielającej aktywną zakładkę od jej zawartości.


## Padding tekstu

Tekstowe kontrolki `BUTTON`, `LABEL`, `DAREA` i `COMBO` obsługują `PADX` i `PADY`.

```nyota
BUTTON zapisz, panel, [140, 38], [20, 20], TEXT="Zapisz", PADX=12, PADY=6
```

Domyślne wartości NyotaUI to `PADX=8` i `PADY=4`.

Główki `TAB` mają osobne `TABPADX` i `TABPADY`; domyślnie odpowiednio 12 i 6. Padding wpływa również na automatycznie wyliczaną szerokość i wysokość główki zakładki.


## Layout: FREE / ROW / COL

`PANEL` i zawartość `TAB` mogą zarządzać pozycjami dzieci.

```nyota
PANEL pasek, glowne, [600, 70], [20, 20], LAYOUT=ROW, GAP=10, LPADX=12, LPADY=12

BUTTON pierwszy, pasek, [120, 36], AUTO, TEXT="Pierwszy"
BUTTON drugi, pasek, [120, 36], AUTO, TEXT="Drugi"
```

`LAYOUT=FREE` zachowuje klasyczne ręczne pozycjonowanie. `ROW` układa dzieci `AUTO` od lewej do prawej, a `COL` od góry do dołu. `GAP` ustala odstęp między elementami, a `LPADX/LPADY` wewnętrzny odstęp kontenera. Dzieci z jawnym `[x,y]` lub `CENTER` nadal mogą współistnieć z elementami `AUTO`.

## Rounded clipping

Przy `CLIP=TRUE` dzieci `PANEL` i `TAB` są na hoście POSIX przycinane również do zaokrąglonego kształtu rodzica. Maskowanie obejmuje tła, tekst, bordery, focus, cienie i prymitywy kontrolek, a nie tylko prostokątny bounding box.

## HiDPI

Host POSIX tworzy okna NyotaUI z obsługą HiDPI i utrzymuje logiczny układ współrzędnych Nyoty przez `SDL_RenderSetLogicalSize`. Kod programu pozostaje niezależny od skali monitora. AyoOS i inne hosty mogą realizować ten sam kontrakt własnym mechanizmem skalowania.

## TAREA / TextBox

`TAREA` jest wielowierszowym polem tekstowym NyotaUI.

```nyota
TAREA opis, panel, [420, 180], AUTO, TEXT="Tekst początkowy"
```

Obsługiwane właściwości obejmują typografię, `BG`, `BORDER`, `RADIUS`, `PADX/PADY`, `WRAP`, `ENABLED`, `READONLY` i kolor kursora `CCARET`.

Na hoście POSIX pole obsługuje UTF-8 przez SDL text input, kliknięcie ustawiające kursor, `Left/Right/Up/Down`, `Home/End`, `Backspace`, `Delete`, `Enter` oraz wklejanie `Ctrl+V`. Focus działa wspólnie z resztą NyotaUI przez `Tab/Shift+Tab`.

Odczyt, ustawienie i sygnał zmiany:

```nyota
VAR tekst := TAREA_TEXT(opis)
VAR ok := TAREA_SET(opis, "Nowa treść")

IF TAREA_CHANGED(opis):
    PRINT "Treść została zmieniona"
```
