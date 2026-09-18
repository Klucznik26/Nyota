# Nyota Language Support for Visual Studio Code

Rozszerzenie VS Code dla języka Nyota.

## Wersja 0.2.1

Rozszerzenie obsługuje:

- rozpoznawanie plików `.nyo`,
- kolorowanie składni Nyoty,
- komentarze, nawiasy i folding oparty na wcięciach,
- przycisk `▶` w edytorze pliku Nyota,
- komendę `Nyota: Run Current File`,
- skrót `Ctrl+F5`,
- automatyczny zapis zmienionego pliku przed uruchomieniem,
- uruchamianie programu w zintegrowanym terminalu VS Code,
- katalog roboczy ustawiany na katalog uruchamianego pliku,
- konfigurowalną ścieżkę interpretera przez `nyota.interpreterPath`.

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
nyota-language-support-0.2.1.vsix
```

Instalacja:

```bash
code --install-extension ./nyota-language-support-0.2.1.vsix --force
```

## Zmiany w 0.2.1

- `Ctrl+F5` uruchamia `Nyota: Run Current File`, gdy aktywny jest edytor Nyoty.
- W innych językach `Ctrl+F5` zachowuje standardowe działanie VS Code: Run Without Debugging.
- Przycisk `▶` w pasku edytora pozostaje dostępny dla plików `.nyo`.
