# do_wdrożenia.md — Nyota

**Ostatnia weryfikacja:** 2026-09-18  
**Status:** kolejka wdrożeniowa zamknięta dla pozycji, które miały ustaloną semantykę  
**Powiązane:** [`nyota.md`](nyota.md), [`nyota_v05.md`](nyota_v05.md)

Ten plik był roboczą kolejką elementów do wdrożenia. Zgodnie z jego pierwotną
zasadą gotowe konstrukcje zostały przeniesione do głównej dokumentacji Nyoty.

## Wdrożone 2026-09-18

### FILE

Działają wysokopoziomowe operacje plikowe przez `NyotaHost`:

```nyota
FILE_WRITE "plik.txt", "tekst"
FILE_APPEND "plik.txt", "dalszy tekst"
FILE_COPY "plik.txt", "kopia.txt"
FILE_MOVE "kopia.txt", "archiwum.txt"
FILE_DELETE "archiwum.txt"

VAR tekst := FILE_READ("plik.txt")
VAR istnieje := FILE_EXISTS("plik.txt")
VAR rozmiar := FILE_SIZE("plik.txt")
```

Rdzeń języka nie używa bezpośrednio POSIX ani struktur systemu plików.
Host POSIX ma gotowy backend. Host AyoOS wymaga podłączenia tych samych callbacków
do AyoAPI/VFS; nie zmienia to składni języka.

### DIR / LS

Działają:

```nyota
DIR_CREATE "dane"
DIR_COPY "dane", "kopia"
DIR_MOVE "kopia", "archiwum"
DIR_DELETE "archiwum"

VAR jest := DIR_EXISTS("dane")
VAR pliki := DIR_LIST("dane")
VAR pliki2 := LS("dane")
```

`DIR_LIST()` i `LS()` są funkcjami i zwracają posortowany `LIST` nazw.
Referencyjny host POSIX kopiuje katalogi rekurencyjnie, ale nie podąża za
symlinkami ani plikami specjalnymi. `DIR_DELETE` usuwa tylko pusty katalog.

### SORT — tryb jawnego algorytmu

Poza podstawowym `SORT lista` i `SORT lista, DESC` działają:

```nyota
SORT dane, AUTO
SORT dane, BUBBLE
SORT dane, INSERT
SORT dane, SELECT
SORT dane, MERGE
SORT dane, QUICK
SORT dane, HEAP
SORT dane, SHELL
SORT dane, COUNTING
SORT dane, QUICK, DESC
SORT dane, QUICK, REVERSE
```

`ASC` jest domyślne. `REVERSE` jest aliasem kierunku malejącego.
`COUNTING` działa dla `INTEGER` i `BOOLEAN`; bieżąca implementacja
ogranicza zakres wartości do 4096 różnych pozycji.

### NYASM / ASM

Działa pierwszy, bezpieczny etap: wirtualny assembler R0–R3 z jawnymi
wejściami i wyjściami.

```nyota
NYASM INPUT a, b OUTPUT wynik:
    MOV R0, a
    ADD R0, b
    STORE wynik, R0
```

`ASM` jest akceptowanym aliasem `NYASM`. Dostępne instrukcje:

```text
MOV ADD SUB MUL DIV MOD STORE
```

NYASM:
- czyta wyłącznie zmienne zadeklarowane w `INPUT`,
- zapisuje wyłącznie zmienne zadeklarowane w `OUTPUT`,
- obsługuje `INTEGER` i `BOOLEAN`,
- sprawdza przepełnienie i dzielenie przez zero,
- nie ma skoków do kodu Nyoty, syscalli, dostępu do stosu ani dowolnej pamięci,
- nie może mutować STRING/LIST/MARK ani stałych.

Natywny x86-64 ASM/XASM **nie jest pozycją do automatycznego wdrożenia**.
W starym dokumencie był tylko możliwością „do rozważenia później”; nie ma
zamkniętej specyfikacji ABI, bezpieczeństwa ani przenośności. Pozostaje w
`nyota_v05.md` jako osobny kierunek FUTURE.

## Testy regresyjne

Dodane testy:

```text
tests/file_ops.nyo
tests/dir_ops.nyo
tests/sort_algorithms.nyo
tests/nyasm_basic.nyo
tests/nyasm_guard.nyo
```

## Co nie jest już kolejką z tego pliku

Pomysły takie jak pełny model uchwytów plików, `DIR_LIST_EX()`, `SORTED()`
czy natywny assembler były w starym pliku opisane jako możliwe późniejsze
rozszerzenia, a nie jako zatwierdzona semantyka do wdrożenia. Nie należy ich
dodawać przez zgadywanie. Po podjęciu decyzji projektowej trafiają najpierw
do specyfikacji, a dopiero potem do kolejki implementacyjnej.
