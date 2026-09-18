<h1 align="center">Nyota</h1>

<p align="center">
  <strong>Autorski język programowania AyoOS — prosty, jawny, przenośny i rozwijany razem z własnym NyotaUI.</strong>
</p>

<p align="center">
  <img alt="Status" src="https://img.shields.io/badge/status-active%20development-orange">
  <img alt="Implementation" src="https://img.shields.io/badge/implementation-C-00599C?logo=c&logoColor=white">
  <img alt="Linux host" src="https://img.shields.io/badge/host-Linux-FCC624?logo=linux&logoColor=black">
  <img alt="AyoOS" src="https://img.shields.io/badge/target-AyoOS-6f42c1">
  <img alt="Source files" src="https://img.shields.io/badge/source-.nyo-blueviolet">
</p>

---

## Czym jest Nyota?

**Nyota** to własny język programowania tworzony dla **AyoOS**, ale projektowany tak, aby sam język nie był zależny od jednego edytora ani jednego systemu operacyjnego.

AyoOS jest pierwszym systemem-hostem Nyoty. Równolegle rozwijany jest host dla Linuksa, a architektura interpretera rozdziela rdzeń języka od warstwy platformowej.

**Tunga** jest osobnym edytorem i nie stanowi części Nyoty. Docelowo ten sam kod `.nyo` ma być uruchamiany przez różne środowiska bez zmiany semantyki języka.

Nyota stawia na:

- czytelną składnię opartą o wcięcia,
- jawne typy i brak ukrytych konwersji,
- `:=` do przypisania i ścisłe `=` do porównania typu oraz wartości,
- proste funkcje i procedury,
- struktury danych przydatne w codziennym programowaniu,
- wbudowane możliwości systemowe i graficzne,
- własny, host-neutralny model GUI **NyotaUI**,
- ten sam kod języka i interfejsu na AyoOS oraz hostach desktopowych.

---

## Krótki przykład

```nyota
FUNCTION Dodaj(a, b):
    RETURN a + b

BEGIN
VAR data := <2026.09.17>
VAR wynik := Dodaj(2, 3)

IF YEAR(data) = 2026:
    PRINT wynik

FOR i := 0 TO 10 STEP 5:
    PRINT i
END
```

Najważniejsze reguły widoczne już w tym przykładzie:

```text
.nyo        rozszerzenie plików źródłowych
BEGIN/END   rama kodu wykonywalnego
4 spacje    jeden poziom wcięcia
:=          przypisanie
=           ścisłe porównanie typu i wartości
```

---

## NyotaUI — GUI jest częścią języka

Nyota rozwija własną warstwę interfejsu **NyotaUI**. Program `.nyo` nie wywołuje bezpośrednio SDL, WinAPI ani AyoAPI — używa kontrolek i właściwości Nyoty, a host realizuje ten sam kontrakt na danej platformie.

Dzięki temu składnia aplikacji pozostaje niezależna od backendu:

```text
program .nyo
    │
    ├── WIN / PANEL / FRAME / TABS
    ├── INPUT / BUTTON / SWITCH / TREEVIEW
    ├── SCALE / CLOCK / EQBOX / PBAR
    └── wspólne kolory, gradienty, obrazy, layout i zdarzenia
            │
            ▼
        NyotaHost
      ┌─────┴─────────────┐
      ▼                   ▼
 POSIX / SDL2          AyoOS
```

Krótki fragment NyotaUI:

```nyota
WIN glowne, ROOT, [1180, 720], CENTER, TRUE

WIN glowne.CONFIG:
    TITLE = "NyotaUI"
    BG = DARKSAPPHIRE

FRAME telemetry, glowne, [520, 300], [30, 80], TEXT="Telemetria"

FRAME telemetry.CONFIG:
    LAYOUT = COL
    GAP = 12
    LPADX = 16
    LPADY = 30
    BORDER = TRUE
    CBORDER = SAPPHIRE
    RADIUS = 10

INPUT search, telemetry, [320, 40], AUTO, TYPE=SEARCH, PLACEHOLDER="Szukaj...", CLEARBUTTON=TRUE
SWITCH wifi, telemetry, [96, 34], AUTO, VALUE=TRUE
SCALE volume, telemetry, [360, 44], AUTO, MIN=0, MAX=100, VALUE=65
```

Aktualny zestaw NyotaUI obejmuje między innymi:

- **okna i kontenery:** `WIN`, `PANEL`, `FRAME`, `TABS/TAB`, `TOOLBAR`, `STATBAR`;
- **wejście i sterowanie:** `BUTTON`, `ICONBUTTON`, `INPUT`, `TBOX`, `TAREA`, `CBOX`, `RADIO`, `COMBO`, `SWITCH`, `SPINBOX`, `SLIDER`, `SBAR`, `SPLITTER`, `DAREA`;
- **prezentację danych:** `LABEL`, `LISTVIEW`, `TREEVIEW`, `TABLE`, `PBAR`, `EQBOX`;
- **wskaźniki i telemetrię:** liniową i obrotową `SCALE` oraz wielowskazówkowy `CLOCK`;
- **warstwę wizualną:** kolory Nyoty, PNG, `GRAD(LINEAR/...)`, `GRAD(SHAPE/...)`, `GRAD(SPIRAL/...)`, promienie, ramki, padding, cienie, poświaty i stany interaktywne;
- **layout:** ręczny `FREE`, automatyczny `ROW/COL`, pozycje `AUTO` i `CENTER`, zaokrąglone przycinanie dzieci oraz logiczne HiDPI.

Backend POSIX używa obecnie SDL2, SDL2_image, SDL2_ttf i fontconfig. Tekst jest renderowany antyaliasingowo w UTF-8, a `FONT="SYSTEM"` korzysta z fontu systemowego hosta. Kontrolki interaktywne obsługują focus klawiatury, `Tab/Shift+Tab`, stany hover/pressed/disabled oraz — tam gdzie ma to sens — sterowanie klawiaturą.

`EQBOX`, `SCALE` i `CLOCK` są projektowane nie tylko jako klasyczne kontrolki formularzy, ale również jako efektowne, szybko aktualizowane elementy telemetryczne i wizualizacyjne.

Pełny kontrakt znajduje się w [`docs/nyotaui.md`](docs/nyotaui.md).

---

## Stan projektu

Nyota jest aktywnie rozwijana. Rdzeń interpretera jest już używalny, ale część bardziej rozbudowanych elementów pozostaje w trakcie stabilizacji.

| Obszar | Stan | Uwagi |
|---|:---:|---|
| `BEGIN / END`, komentarze, 4-spacjowe wcięcia | ✅ | interpreter egzekwuje strukturę programu |
| Wyrażenia i precedencja operatorów | ✅ | m.in. `*`, `/`, `+`, `-`, `MOD`, nawiasy, logika |
| `IF / ELIF / ELSE` | ✅ | jeden spójny łańcuch warunkowy |
| `FOR ... STEP`, `WHILE`, `CONTINUE` | ✅ | podstawowe sterowanie przepływem |
| `FUNCTION`, `PROCEDURE`, `RETURN`, zakresy | ✅ | wywołania funkcji działają także w wyrażeniach |
| Ścisłe typowanie i jawne konwersje | 🚧 | system typów jest intensywnie dopracowywany |
| `DATE` | ✅ | literał `<RRRR.MM.DD>`, walidacja gregoriańska i arytmetyka dni |
| `LIST` | 🚧 | rozszerzane operacje i semantyka kolekcji |
| `MARK` | ✅ | klucze+kolumny, algebra, iteracja, sortowanie, przebudowa kolumn i statystyki |
| `TABLE` | ✅ | nazwana kontrolka prezentacji LIST/TUPLE/MARK w trybie graficznym |
| `BUTTON` | ✅ | nazwana kontrolka GUI; wygląd + wykrywanie kliknięcia na hoście POSIX |
| `WIN / NyotaUI` | 🚧 | rozbudowany host-neutralny toolkit: kontenery, wejście, listy/drzewa, layout, telemetria, gradienty, obrazy, UTF-8, focus i HiDPI; backend POSIX jest aktywnie rozwijany |
| `SPRITE` | ✅ | nazwany obiekt graficzny; ruch, widoczność, animacja klatkowa, jawne rysowanie i kolizja AABB |
| `FILE / DIR / LS` | ✅ | wysokopoziomowy kontrakt hosta; pełny backend POSIX |
| `RECORD / WITH` | ✅ | rekordy z blokadą typów i kontekstem pól |
| `IMPORT` | ✅ | bezpieczny import deklaracji przed BEGIN |
| `EVERY / ON ERROR` | ✅ | scheduler kooperacyjny i handler błędów |
| `SCREEN` | ✅ | cele off-screen; backend POSIX/SDL2 |
| `SORT` | ✅ | AUTO + 8 jawnych algorytmów, ASC/DESC/REVERSE |
| `NYASM` | ✅ | bezpieczna VM R0-R3 z jawnym INPUT/OUTPUT |
| Host Linux | 🚧 | terminal + SDL2 / SDL2_image / SDL2_ttf / fontconfig dla grafiki, NyotaUI i FILE/DIR |
| Host AyoOS | 🚧 | docelowo wspólny `src/nyota.c` dla wszystkich hostów |
| VS Code | ✅ | Nyota Language Support 0.5.3: kolorowanie + uruchamianie przez `▶` / `Ctrl+F5` + próbki/picker kolorów NyotaUI |

Szczegółowy stan interpretera znajduje się w [`docs/nyota.md`](docs/nyota.md), a droga do stabilizacji i rozwoju w [`docs/nyota_v05.md`](docs/nyota_v05.md).

---

## Budowanie na Linuksie

Aktualny host POSIX/Linux jest budowany z kodu C i korzysta z SDL2.

### Wymagania

```text
kompilator C zgodny z C11 (np. GCC)
make
pkg-config
SDL2 wraz z plikami deweloperskimi
SDL2_image wraz z plikami deweloperskimi
SDL2_ttf wraz z plikami deweloperskimi
fontconfig wraz z plikami deweloperskimi
```

### Kompilacja

```bash
git clone https://github.com/Klucznik26/Nyota.git
cd Nyota
git switch nyota-v05-complete
make
```

Powstanie plik wykonywalny:

```text
./nyota
```

### Uruchomienie programu

```bash
./nyota program.nyo
```

Na przykład:

```bash
./nyota tests/core_demo.nyo
```

Aktualny program hosta przyjmuje plik `.nyo` jako pierwszy argument.

---

## Testy

Repozytorium zawiera zestaw programów regresyjnych w katalogu `tests/`.

```bash
make test
```

Testy obejmują między innymi parser wyrażeń, typy, błędy składni i wykonania, funkcje, pętle, daty, listy oraz rozwijany `MARK`.

Bez `GRAPH` program pisze na stdout. Po `GRAPH 1..6` otwiera się okno SDL.
`PRINT` po `GRAPH` jest błędem języka. Okno czeka na ESC.

```bash
./nyota tests/graph_box.nyo
make test-graph
NYOTA_INPUT=hello ./nyota tests/input_hello.nyo
```

Duży test demonstracyjny rdzenia:

```bash
./nyota tests/core_demo.nyo
```

## RPM (Fedora)

Pakiet jest budowany i testowany natywnie w środowisku Fedora 44. Do lokalnego
zbudowania potrzebne są narzędzia RPM i nagłówki zgodności SDL2:

```bash
sudo dnf install rpm-build gcc make pkgconf-pkg-config sdl2-compat-devel SDL2_image-devel SDL2_ttf-devel fontconfig-devel tar gzip
make rpm
sudo dnf install packaging/nyota-0.5.0-23.fc44.x86_64.rpm
nyota /usr/share/nyota/tests/add_int.nyo
```

`make rpm` tworzy zarówno RPM binarny, jak i SRPM w katalogu `packaging/`.
Fedora 44 dostarcza `pkgconfig(sdl2)` przez `sdl2-compat-devel`; NyotaUI korzysta także z SDL2_image, SDL2_ttf i fontconfig. Zależności runtime są wykrywane automatycznie przez RPM. Pakiet zawiera
również programy testowe, zasób BMP do testów SPRITE oraz dokumentację projektu.

Workflow `Fedora RPM` buduje pakiet na Fedorze 44, instaluje go w czystym
kontenerze i wykonuje test uruchomieniowy. Gotowe RPM-y są zapisywane jako
artefakt workflow `nyota-fedora-44-rpm`.

---

## Architektura

```text
Nyota source (.nyo)
        │
        ▼
  src/nyota.c
  interpreter core
        │
        ▼
 src/nyota_host.h
  host contract
      ┌─┴───────────────┐
      ▼                 ▼
 host/posix/        host/ayoos/
 Linux + SDL2       AyoOS
```

Rdzeń interpretera jest oddzielany od platformy. Polecenia języka takie jak `PRINT`, `GRAPH`, `INPUT` czy `DELAY` należą do Nyoty; host dostarcza jedynie ich backend dla danego systemu.

Interpreter udostępnia również punkt wejścia do osadzania:

```c
void NyotaEmbedRun(const char *src, void (*emit)(char c));
```

Dzięki temu Nyota może być uruchamiana z różnych edytorów i środowisk bez tworzenia osobnej odmiany języka.

---

## Struktura repozytorium

```text
src/
    nyota.c          interpreter
    nyota_host.h     kontrakt hosta

host/
    posix/           host Linux / SDL2
    ayoos/           integracja AyoOS

docs/
    nyota.md         bieżąca specyfikacja i stan interpretera
    nyota_v05.md     plan stabilizacji i rozwoju
    do_wdrożenia.md  dalsze elementy do wdrożenia

tests/               programy testowe .nyo
scripts/             automatyzacja testów
editors/vscode/      rozszerzenie VS Code: .nyo + kolorowanie + uruchamianie
Makefile             budowanie hosta Linux
```

---

## Dokumentacja

Najważniejsze dokumenty projektu:

- [`docs/nyota.md`](docs/nyota.md) — bieżące zasady języka i faktyczny stan interpretera,
- [`docs/nyota_v05.md`](docs/nyota_v05.md) — plan stabilizacji oraz zatwierdzone kierunki rozwoju,
- [`docs/nyotaui.md`](docs/nyotaui.md) — pełny kontrakt NyotaUI: okna, kontenery, kontrolki, layout, wejście tekstowe, listy/drzewa, `SCALE`, `CLOCK`, `EQBOX`, obrazy, gradienty, focus, HiDPI i warstwa wizualna,
- [`docs/do_wdrożenia.md`](docs/do_wdrożenia.md) — elementy oczekujące na implementację,
- [`docs/zasadymd.md`](docs/zasadymd.md) — sposób oznaczania stanu prac w dokumentacji.

---

## Kierunek rozwoju

Po ustabilizowaniu rdzenia Nyota ma rozwijać się dalej w stronę bogatszych struktur danych, `DATETIME`, relacyjnego `MARK`, dalszego rozwoju `TABLE`, audio, sprite'ów oraz kolejnych możliwości NyotaUI i hostów.

Poza AyoOS działa rozszerzenie **Nyota Language Support 0.5.3** dla **Visual Studio Code**: rozpoznaje `.nyo`, koloruje składnię, automatycznie normalizuje wpisane polecenie `eqbox` do `EQBOX` i uruchamia aktualny program przez przycisk `▶`, `Ctrl+F5` albo komendę `Nyota: Run Current File`. Interpreter jest pobierany z `PATH` lub z ustawienia `nyota.interpreterPath`. Wersja 0.5.3 zna pełną składnię v0.5 oraz rozwiniętą składnię NyotaUI `WIN`, `BUTTON`, `LABEL`, `PANEL`, `DAREA`, `CBOX`, `RADIO`, `COMBO`, `SEP`, `TABS`, `TAB`, `TAREA`, `SBAR`, `PBAR`, `EQBOX`, `SLIDER`, `STATBAR`, `TOOLBAR`, `TBOX`, `INPUT`, `ICONBUTTON`, `SWITCH`, `FRAME`, `SPINBOX`, `LISTVIEW`, `TREEVIEW`, `SPLITTER`, `SCALE`, `CLOCK`, `IMG` i `GRAD`, w tym FILE/DIR, SCREEN, RECORD/WITH, IMPORT, EVERY, obsługę błędów, jawne algorytmy SORT i NYASM. Kolejne etapy mogą dodać diagnostykę i podpowiedzi. Nadal planowane są też pakiety Linuksa — w pierwszej kolejności dla **Fedory** i **openSUSE**.

Nyota nie ma zastępować C lub Zig w najniższych warstwach systemu. Jej celem jest wygodne tworzenie aplikacji, narzędzi, automatyzacji, grafiki i prostych gier przy zachowaniu własnej, spójnej semantyki.

---

<p align="center">
  <strong>Nyota jest częścią świata AyoOS, ale język ma żyć także poza nim.</strong>
</p>
