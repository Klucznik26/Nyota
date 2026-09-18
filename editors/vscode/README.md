# Nyota Language Support for Visual Studio Code

Rozszerzenie VS Code dla języka Nyota.

## Wersja 0.4.2

Rozszerzenie obsługuje:

- rozpoznawanie plików `.nyo`,
- kolorowanie składni Nyoty, w tym FILE/DIR, algorytmów SORT i NYASM,
- komentarze, nawiasy i folding oparty na wcięciach,
- przycisk `▶` w edytorze pliku Nyota,
- komendę `Nyota: Run Current File`,
- skrót `Ctrl+F5`,
- automatyczny zapis zmienionego pliku przed uruchomieniem,
- uruchamianie programu w zintegrowanym terminalu VS Code,
- katalog roboczy ustawiany na katalog uruchamianego pliku,
- konfigurowalną ścieżkę interpretera przez `nyota.interpreterPath`,
- próbki kolorów (swatches) i standardowy picker VS Code dla 98 nazwanych kolorów Nyoty, `TRANSPARENT`, `0xRRGGBB` i `0xRRGGBBAA`,
- zachowanie `BACKDROP` bez próbki koloru, ponieważ jego wynik zależy od sceny pod obiektem,
- kolorowanie składni NyotaUI: `WIN`, `ROOT`, `CENTER`, `CONFIG`, `IMG(...)`, `GRAD(...)` oraz trybów obrazów i gradientów.

Domyślna wartość interpretera to:

```text
nyota
```

Dzięki temu na systemie, na którym pakiet Nyoty zainstalował `/usr/bin/nyota`,
nie trzeba nic konfigurować.

## Uruchamianie

Otwórz plik `.nyo`, a następnie:

- kliknij `▶` w prawym górnym rogu edytora,
- albo naciśnij `Ctrl+F5`,
- albo wybierz z palety poleceń `Nyota: Run Current File`.

Wynik programu pojawi się w terminalu VS Code.

## Ręczna ścieżka interpretera

W ustawieniach VS Code można ustawić np.:

```json
"nyota.interpreterPath": "/usr/bin/nyota"
```

Zwykle nie jest to potrzebne, jeśli `nyota` znajduje się w `PATH`.

## Zbudowanie pliku VSIX

```bash
cd editors/vscode
npx @vscode/vsce package
```

Dla tej wersji powstanie plik:

```text
nyota-language-support-0.4.2.vsix
```

Instalacja:

```bash
code --install-extension ./nyota-language-support-0.4.2.vsix --force
```

## Zmiany w 0.4.2

- składnia `WIN` i `WIN nazwa.CONFIG:`,
- słowa NyotaUI: `ROOT`, `CENTER`, `TITLE`, `BG`, `ICO`,
- funkcje tła `IMG(...)` i `GRAD(...)`,
- kierunki, kształty, tryby obrazów oraz `CW` / `CCW`.

## Zmiany w 0.4.1

- kolorowe próbki obok nazwanych kolorów NyotaUI i zapisów szesnastkowych,
- kliknięcie próbki otwiera standardowy selektor koloru VS Code,
- picker potrafi zapisać dokładnie dopasowany kolor jako nazwę Nyoty albo jako `0xRRGGBB` / `0xRRGGBBAA`,
- `TRANSPARENT` jest obsługiwany z alfą 0,
- `BACKDROP` celowo nie otrzymuje próbki, bo nie reprezentuje pojedynczego RGB,
- paleta rozszerzenia jest testowana na zgodność z `src/nyota_color.h`.

## Zmiany w 0.4.0

- pełna precedencja operatorów `!`, `SHL`, `SHR`, `BAND`, `BXOR`, `BOR`,
- aktualna składnia `RECORD`, `WITH`, `IMPORT`, `EVERY`, `SCREEN`,
- systemowy `ERR_CODE`,
- komplet instrukcji bezpiecznej VM NYASM.

## Zmiany w 0.3.0

- kolorowanie nowych funkcji i instrukcji `FILE_*` oraz `DIR_*`,
- kolorowanie `LS()`,
- kolorowanie jawnych algorytmów `SORT`,
- kolorowanie `NYASM` / `ASM`, `INPUT`, `OUTPUT` oraz instrukcji VM.

## Zmiany w 0.2.1

- `Ctrl+F5` uruchamia `Nyota: Run Current File`, gdy aktywny jest edytor Nyoty.
- W innych językach `Ctrl+F5` zachowuje standardowe działanie VS Code: Run Without Debugging.
- Przycisk `▶` w pasku edytora pozostaje dostępny dla plików `.nyo`.
