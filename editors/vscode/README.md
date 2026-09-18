# Nyota Language Support for Visual Studio Code

Pierwsza wersja rozszerzenia VS Code dla języka Nyota.

Zakres v0.1:

- rozpoznawanie plików `.nyo`,
- kolorowanie komentarzy `#`,
- stringów i znacznika nowej linii `~/`,
- liczb, literałów `DATE`, `TIME` i przesunięć `Hn/Mn/Sn`,
- słów kluczowych sterowania przepływem,
- deklaracji `VAR`, `CONST`, `FUNCTION`, `PROCEDURE`, `RECORD`,
- funkcji wbudowanych Nyoty,
- poleceń graficznych i GUI,
- `TABLE`, `BUTTON`, `SPRITE` wraz z nazwami logicznymi,
- operatorów Nyoty, w tym `:=`, `=N=`, `<>` i `><`,
- podstawowej konfiguracji nawiasów i komentarzy.

Rozszerzenie jest deklaratywne: nie zawiera kodu JavaScript ani TypeScript.

## Uruchomienie w trybie deweloperskim

1. Otwórz katalog `editors/vscode` w Visual Studio Code.
2. Naciśnij `F5`.
3. W nowym oknie Extension Development Host otwórz dowolny plik `.nyo`.

## Zbudowanie pliku VSIX

Jeżeli masz `@vscode/vsce`:

```bash
cd editors/vscode
npx @vscode/vsce package
```

Powstanie plik podobny do:

```text
nyota-language-support-0.1.0.vsix
```

Można go zainstalować lokalnie:

```bash
code --install-extension nyota-language-support-0.1.0.vsix
```

## Zakres

Ta wersja daje wyłącznie obsługę języka po stronie edytora: identyfikację `.nyo`
i kolorowanie składni. Diagnostyka interpretera, uruchamianie Nyoty z VS Code,
podpowiedzi składni, hover i autouzupełnianie mogą dojść później.
