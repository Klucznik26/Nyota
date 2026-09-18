# Nyota v0.5 — plan stabilizacji i rozwoju

**Ostatnia weryfikacja całości:** 2026-09-18  
**Powiązane:** [`nyota.md`](../Programs/Tools/nyota/nyota.md) — bieżąca specyfikacja dla agenta i stan interpretera; ten plik jest planem wydania, nie źródłem prawdy o tym, co już działa.

**Status:** rdzeń i zatwierdzona semantyka wdrożone; dokument zachowany jako historia decyzji  
**Wersja dokumentu:** 0.5  
**Data:** 2026-09-17  
**Dotyczy:** przejścia od obecnego stanu Nyoty do wydania v0.5
oraz zatwierdzonego rozwoju po v0.5

> Ten dokument nie jest bieżącą specyfikacją języka.  
> Opisuje decyzje projektowe, zmiany, naprawy, zagrożenia i nowe elementy.
>
> Zatwierdzenie pomysłu nie oznacza, że interpreter dostaje go w v0.5.
> Każdy element ma jeden status wdrożenia z działu 2.

---

# 1. O Nyota

Nyota jest autorskim językiem programowania AyoOS.

Język ma być prosty do czytania i szybki w użyciu, ale jednocześnie nie powinien
opierać się na ukrytych konwersjach ani zgadywaniu intencji programisty.

Nyota czerpie inspiracje z kilku języków, ale nie ma być kopią żadnego z nich.
Jej charakter mają budować między innymi:

- blokowa składnia oparta o wcięcia,
- `:=` jako przypisanie,
- `=` jako ścisłe porównanie typu i wartości,
- funkcje i procedury,
- typ wnioskowany przy deklaracji, ale później blokowany,
- proste instrukcje graficzne i systemowe,
- własne struktury i operatory,
- ścisła integracja z AyoOS.

## 1.1. Podstawowa filozofia

Nyota powinna być czytelna, jawna tam gdzie istnieje ryzyko niejednoznaczności,
nie wykonywać zaskakujących automatycznych konwersji typów i rozwijać się etapami
bez dokładania nowych elementów kosztem stabilności rdzenia.

## 1.2. Typowanie

Typ zmiennej jest wnioskowany podczas deklaracji.

```nyota
VAR liczba := 5
VAR tekst := "Ayo"
VAR aktywny := TRUE
```

Po ustaleniu typ zmiennej nie może zostać zmieniony:

```nyota
VAR x := 10
x := 20          # OK
x := "tekst"     # BŁĄD
```

Zmiana reprezentacji wartości wymaga jawnej konwersji.

## 1.3. Instrukcje i funkcje

Nyota zachowuje rozróżnienie:

- instrukcja wykonuje akcję i nie zwraca wartości,
- funkcja zwraca wartość i jest wywoływana z nawiasami.

```nyota
PRINT "AyoOS"
VAR dlugosc := LEN("AyoOS")
```

## 1.4. Porównanie

`=` porównuje jednocześnie typ i wartość. Różne typy to błąd, a nie `FALSE`.

```nyota
5 = 5            # TRUE
"5" = "5"        # TRUE
5 = 6            # FALSE
5 = 5.0          # BŁĄD TYPU
5 = "5"          # BŁĄD TYPU
TRUE = 1         # BŁĄD TYPU
```

Porównanie wartości o różnych typach wymaga jawnej konwersji:

```nyota
FLT(5) = 5.0
INT(5.0) = 5
```

`==` i `===` nie istnieją. Świadomym wyjątkiem liczbowym pozostaje `=N=`,
opisany w dziale o nowych elementach i w aneksie tokenizacji.

---

# 2. Podział wdrożenia

Ten dział jest nadrzędny wobec listy pomysłów w dalszej części dokumentu.

Zatwierdzona semantyka i wydanie v0.5 to dwie różne rzeczy. v0.5 ma
ustabilizować rdzeń. Rozbudowa LIST, MARK, DATE, TIME i dalsze kierunki
mogą być zaprojektowane wcześniej, ale nie blokują i nie wypełniają v0.5.

## 2.1. Statusy

Każdy element ma dokładnie jeden status wdrożenia:

```text
BLOCKS v0.5
    Bez tego v0.5 nie wychodzi.

APPROVED AFTER CORE
    Semantyka zatwierdzona. Implementować po naprawie rdzenia,
    nie w ramach zamknięcia v0.5.

FUTURE
    Kierunek zatwierdzony koncepcyjnie, ale jeszcze projektowany
    albo świadomie odłożony po ustabilizowaniu v0.5.
```

Status `BLOCKS v0.5` dotyczy interpretera, testów i tej części dokumentacji,
bez której wydanie kłamałoby o działającym języku.

## 2.2. BLOCKS v0.5

Bez poniższych punktów v0.5 nie zostaje wydane:

```text
parser wyrażeń, precedencja i łączność
FUNCTION / PROCEDURE / parametry / RETURN
wywołania FUNCTION w wyrażeniach
zakresy zmiennych i przesłanianie
IF / ELIF / ELSE jako jeden łańcuch
wcięcia: 4 spacje i twarda walidacja
egzekwowanie BEGIN / END
jawne błędy zamiast cichego ignorowania
typowanie: wnioskowanie przy deklaracji, potem blokada
jawne konwersje INT / FLT / STR / BOOL
BOOLEAN bez udziału w arytmetyce
= jako ścisłe porównanie typu i wartości; brak == i ===
brak niejawnych konwersji w operatorze +
dzielenie przez zero i MOD 0 jako błąd
podstawy FLOAT: literały, działania, dzielenie, liczby ujemne
zestaw operatorów matematycznych: % procent, MOD, ^, ^^
WHILE, CONTINUE, STEP
testy regresyjne dla każdej reguły BLOCKS
lexer rozpoznaje tokeny z aneksu C i odrzuca je jawnym błędem
    zamiast nadawać im tymczasowe znaczenie
```

Istniejące elementy poza tą listą (`RECORD`, `WITH`, `TUPLE`, `GRAPH`,
`SCREEN`, `IMPORT`, `EVERY`, obsługa błędów programu, grafika) zostają
w v0.5 bez przebudowy. Naprawa tylko wtedy, gdy bieżący interpreter
łamie już zapisaną specyfikację.

## 2.3. APPROVED AFTER CORE

Semantyka jest zatwierdzona. Wdrożenie następuje po wydaniu albo po
twardym zamknięciu rdzenia, nie jako warunek v0.5:

```text
=N= jako specjalne porównanie liczb po ucięciu
PRINT z wieloma argumentami
REMOVE po indeksie i po wartości
>< jako różnica symetryczna LIST / MARK
bogatszy LIST: +, -, ><, EXTEND, CLEAR, REVERSE
RANDINT / RANDFLT
podstawowy SORT: SORT lista  oraz  SORT lista, DESC
rozbudowany MARK: REKEY, KEY/VALUE/VALUES, IN, FOR IN,
    +, EXTEND, -, ><, MINFO, MLIST, MEXTEND, MINSERT, MDROP,
    SUM / AVG / MED / MIN / MAX / MODE / MODECOUNT / COUNT
DATE z literałem <RRRR.MM.DD> i arytmetyką dni
TIME(HH.MM.SS) oraz TIME +/- Hn/Mn/Sn
    — implementacja dopiero po zamknięciu TIME - TIME
      i decyzji o DATETIME
FILE i DIR / LS przez NyotaHost / AyoAPI / VFS
    — rdzeń + backend POSIX wykonane 2026-09-18; AyoOS czeka na podłączenie callbacków VFS
```

Dla `TIME` kierunek składni pozostaje zatwierdzony, ale interpreter nie
dostaje typu `TIME`, dopóki nie będzie zapisane: różnica `TIME - TIME`,
zawijanie doby oraz to, czy Nyota wprowadza `DATETIME`.

## 2.4. FUTURE

Kierunek zatwierdzony koncepcyjnie; specyfikację można projektować
wcześniej, wdrożenie nie należy nawet do fali po rdzeniu v0.5:

```text
wybór algorytmu SORT jako składnia języka (BUBBLE, QUICK, ...) — wykonane 2026-09-18
relacyjny MARK
DATETIME
standard GUI Nyoty
dalszy rozwój sprite'ów (podstawowy SPRITE i animacja klatkowa są już wdrożone)
audio
NYASM — bezpieczna VM wykonana 2026-09-18; assembler natywny nadal FUTURE
Nyota poza AyoOS: Linux i VS Code działają; Windows pozostaje celem hosta
```

Jawne `SORT dane, BUBBLE` ma sens dydaktyczny. Dla użytkownika języka
ważne jest:

```nyota
SORT dane
SORT dane, DESC
```

Algorytm użyty przez interpreter jest szczegółem implementacji, dopóki
osobna decyzja nie wprowadzi trybu dydaktycznego.

## 2.4a. Nyota jest językiem; Tunga i AyoOS nie są Nyotą

Decyzja zamknięta.

Nyota jest niezależnym językiem. Tunga jest osobnym projektem (edytor).
AyoOS jest pierwszym systemem, na którym Nyota działa. Linux i Windows
dostaną ten sam język, w tym VS Code jako edytor — analogicznie do Tungi.

`PRINT`, `GRAPH`, `INPUT` i `DELAY` **należą do języka**. Muszą działać
również na Linuksie. Host nie wycina tych poleceń; podłącza backend
(AyoAPI, SDL, terminal+okno).

```text
1. Tunga ≠ Nyota. Edytor tylko uruchamia interpreter.
2. Jeden język, wiele hostów (AyoOS, Linux, Windows).
3. PRINT, GRAPH, INPUT, DELAY, TODAY są w języku na każdym hoście.
4. Interpreter nie może być przyszyty do AyoAPI; AyoAPI to jeden backend.
5. Nie będzie dialektu bez grafiki.
```

Kontrakt hosta: `Programs/Tools/nyota/nyota_host.h`.

## 2.5. Kolejność pracy

```text
1. Zamknąć specyfikację elementów BLOCKS v0.5, w tym = i tokenizację rdzenia.
2. Naprawić parser, typy, zakresy, funkcje, procedury i błędy.
3. Dodać WHILE, CONTINUE, STEP.
4. Testy regresyjne i synchronizacja dokumentacji v0.5.
5. Wydanie v0.5.
6. Dopiero potem APPROVED AFTER CORE, zaczynając od tokenizacji
   konstrukcji z aneksu C.
7. FUTURE pozostaje projektem, nie kolejką implementacji.
```

Zanim konstrukcje z aneksu C trafią do kodu C interpretera, ich reguły
tokenizacji muszą być zapisane i zamknięte. Nie ustala się ich przy okazji
kolejnego ficzera.

---

# 3. Zmiany do wdrożenia

Ten dział zawiera elementy, które **już istnieją w Nyocie**, ale ich obecne
zachowanie, składnia lub definicja mają zostać zmienione.

Status wdrożenia jest podany przy każdym punkcie. Zmiana istniejącej
konstrukcji nie oznacza automatycznie, że należy do BLOCKS v0.5.

## 3.1. Wcięcia: 4 spacje zamiast 7

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">interpreter: dokładne +4 spacje przy wejściu w blok, tabulator i skoki poziomów są błędem wykonane 2026-09-18</span>  
Pomoc Tunga/AyoEdit znajduje się poza dostępnym repozytorium Nyoty; źródłem prawdy jest reguła 4 spacji.

Jeden poziom bloku Nyoty ma mieć dokładnie **4 spacje**.

```nyota
IF x > 0:
    PRINT "Dodatnia"
    IF x > 100:
        PRINT "Duza"
```

Tabulator nie powinien być częścią składni pliku źródłowego. Edytor może
zamieniać klawisz Tab na 4 spacje.

Zrobione w interpreterze, `nyota.md`, `nyota.txt` i `tests/*.nyo`.
Do zmiany zostaje pomoc Tunga / AyoEdit.

## 3.2. Modulo: `MOD` zamiast `/%`

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">MOD w parserze; /% nadal alias; % zostaje procentem wykonane 2026-09-17</span>

Operator `%` pozostaje operatorem procentu.

```nyota
VAR rabat := 200 % 15
```

Modulo ma być zapisywane:

```nyota
10 MOD 3
```

Docelowy zestaw:

```text
%     procent
MOD   modulo
^     potegowanie
^^    pierwiastkowanie
```

## 3.3. `^^` pozostaje operatorem pierwiastkowania

**Status wdrożenia:** BLOCKS v0.5

```nyota
9 ^^ 2
27 ^^ 3
```

`A ^^ N` oznacza pierwiastek stopnia `N` z wartości `A`.

Do jednoznacznego zdefiniowania pozostają przypadki brzegowe:
stopień 0, stopień ujemny, parzysty pierwiastek z liczby ujemnej,
typ wyniku i kolejność działań względem `^`.

## 3.4. Jawne konwersje typów

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">INT / FLT / STR / BOOL oraz brak niejawnych konwersji wykonane</span>

Nyota nie powinna automatycznie zgadywać, czy programista chce wykonać działanie
liczbowe, czy tekstowe.

```nyota
VAR a := 4
VAR b := "8"
```

To powinno być błędem:

```nyota
VAR c := a + b
```

Poprawnie:

```nyota
VAR c := a + INT(b)
VAR d := STR(a) + b
```

Docelowe funkcje konwersji:

```text
INT(x)    -> INTEGER
FLT(x)    -> FLOAT
STR(x)    -> STRING
BOOL(x)   -> BOOLEAN
```

## 3.5. Konwersja BOOLEAN

**Status wdrożenia:** BLOCKS v0.5

```nyota
INT(TRUE)    # 1
INT(FALSE)   # 0

BOOL(0)      # FALSE
BOOL(1)      # TRUE
BOOL(2)      # TRUE
BOOL(-5)     # TRUE
```

Zasada:

```text
0 -> FALSE
wartosc rozna od 0 -> TRUE
```

`BOOL()` normalizuje wartość. Nie przechowuje wartości wejściowej.

```nyota
VAR b := BOOL(5)
INT(b)       # 1
```

Jeżeli:

```nyota
VAR b := TRUE
```

to `INT(b)` zawsze zwraca `1`, dopóki `b` ma wartość `TRUE`.

BOOLEAN nie powinien automatycznie uczestniczyć w arytmetyce.

```nyota
VAR b := TRUE
VAR a := 2

VAR c := a + INT(b)              # 3
VAR d := INT(BOOL(a)) + INT(b)   # 2
```

## 3.6. Operatory równości

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">`=` i `<>` są ścisłe typowo; `==` i `=N=` są odrzucane jawnym błędem wykonane 2026-09-17</span>

Decyzja jest zamknięta. `=` pozostaje ścisłe typowo, zgodnie z obecną
filozofią Nyoty oraz z regułami już przyjętymi dla zmiennych, `LIST` i `MARK`.

```text
:=    przypisanie
=     ścisłe porównanie: ten sam typ + wartość
<>    nierówność
=N=   specjalne porównanie liczb po obcięciu do N miejsc
      (semantyka zatwierdzona, wdrożenie APPROVED AFTER CORE)
```

`==` i `===` nie istnieją i nie wejdą do Nyoty.

```nyota
5 = 5            # TRUE
"5" = "5"        # TRUE
5 = 6            # FALSE

5 = 5.0          # BŁĄD TYPU
5 = "5"          # BŁĄD TYPU
TRUE = 1         # BŁĄD TYPU
```

Nyota nie zgaduje, czy `5` i `5.0` „właściwie znaczą to samo”.
Porównanie takich wartości wymaga jawnej konwersji:

```nyota
FLT(5) = 5.0
INT(5.0) = 5
```

`=N=` nie jest ukrytą promocją typów. To jawny operator liczbowy,
widoczny w kodzie, opisany w dziale 6.1 i w aneksie C.

Ta decyzja jest zgodna z obecnym `nyota.md`: operator `=` porównuje
wartość i typ, a `==` nie istnieje. Plan v0.5 tego nie odwraca.

## 3.7. REMOVE — rozdzielenie indeksu i wartości

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">REMOVE lista[i], REMOVE lista, wartosc oraz ALL wykonane 2026-09-17</span>

Obecna forma `REMOVE lista, liczba` jest niejednoznaczna dla list liczbowych, bo liczba może oznaczać indeks albo wartość.

Docelowo:

```nyota
REMOVE lista[index]          # usuniecie po indeksie
REMOVE lista, wartosc        # pierwsze wystapienie wartosci
REMOVE lista, wartosc, ALL   # wszystkie wystapienia wartosci
```

Porównanie wartości przy usuwaniu jest ścisłe typowo.

## 3.8. `><` — zmiana znaczenia operatora

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">LIST >< LIST i MARK >< MARK to różnica symetryczna; skalarne >< jest błędem wykonane 2026-09-18</span>

Do v0.5 operator `><` pozostaje aliasem nierówności, tak jak w bieżącej
specyfikacji, **gdy oba operandy nie są listami**. Dla dwóch `LIST` ma
już znaczenie różnicy symetrycznej.

Dotychczasowe użycie `><` jako alternatywnego zapisu nierówności ma zostać usunięte.
Nierówność pozostaje zapisywana jako:

```nyota
a <> b
```

Operator `><` otrzymuje nowe, jednoznaczne znaczenie: **różnica symetryczna**
dla `LIST` i `MARK`.

```nyota
VAR c := a >< b
```

Dla `LIST` operacja uwzględnia liczbę wystąpień elementów. Dla `MARK` działa
na kluczach i pozostawia wpisy występujące tylko w jednym z operandów.

## 3.9. PRINT z wieloma argumentami

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">PRINT a, b, c ze spacją, konwersja tylko do wyświetlenia wykonane 2026-09-17</span>

`PRINT` ma przyjmować wiele argumentów różnych podstawowych typów. Konwersja
do tekstu odbywa się wyłącznie na potrzeby wyświetlenia i nie oznacza
wprowadzenia ogólnych niejawnych konwersji typów.

```nyota
PRINT "Ayo", 5, TRUE
PRINT KEY(kraje, 2), " : ", VALUE(kraje, 2, 1)
```

Nadal błędne pozostaje np.:

```nyota
VAR x := "Ayo" + 5
```

---

# 4. Naprawy

Ten dział zawiera elementy już istniejące i zdefiniowane, które w obecnym
interpreterze działają błędnie, niepełnie albo niezgodnie z dokumentacją.

Wszystkie punkty tego działu mają status **BLOCKS v0.5**.

## 4.1. Wywołania FUNCTION w wyrażeniach

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">wywołanie FUNCTION w wyrażeniu zwraca wartość RETURN wykonane 2026-09-17</span>

Interpreter rozpoznaje funkcję użytkownika w wyrażeniu, ale ścieżka wykonania
wymaga pełnej realizacji wywołania i obsługi wartości `RETURN`.

```nyota
FUNCTION Dodaj(a, b):
    RETURN a + b

BEGIN
VAR wynik := Dodaj(2, 3)
END
```

Oczekiwane: `wynik = 5`.

## 4.2. Parametry PROCEDURE i FUNCTION

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">parametry, VAR, liczba argumentów i lokalne parametry wykonane 2026-09-17</span>

Do pełnego wdrożenia i naprawy:

- zwykłe parametry,
- parametry `VAR`,
- liczba argumentów,
- zgodność typów,
- lokalne zmienne parametrów,
- błędy braku lub nadmiaru argumentów.

## 4.3. Zakres zmiennych i przesłanianie

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">lokalne VAR przesłania globalne; po powrocie globalne zostają wykonane 2026-09-17</span>

Zmienne lokalne muszą poprawnie przesłaniać globalne.

```nyota
VAR x := 1

PROCEDURE Test():
    VAR x := 2
    PRINT x

BEGIN
Test()
PRINT x
END
```

Oczekiwane:

```text
2
1
```

## 4.4. Precedencja i łączność operatorów

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">`2 * 3 + 4 = 10`, `2 + 3 * 4 = 14`, `10 - 3 - 2 = 5`, nawiasy, NOT/AND/OR i porównania w parserze wyrażeń wykonane 2026-09-17</span>  
<span style="color: #006A4E;">SHL/SHR/BAND/BXOR/BOR i silnia `!` w pełnym parserze precedencji wykonane 2026-09-18</span>

```nyota
2 * 3 + 4
```

powinno dać `10`.

```nyota
10 - 3 - 2
```

powinno dać `5`.

Należy zbudować formalną tabelę precedencji obejmującą m.in. `NOT`, `^`, `^^`,
`*`, `/`, `MOD`, `%`, `+`, `-`, porównania, `AND`, `OR` i przyszły `=N=`.

## 4.5. IF / ELIF / ELSE

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">łańcuch IF/ELIF/ELSE wykonuje tylko pierwszą prawdziwą gałąź; osierocony ELIF/ELSE to błąd wykonane 2026-09-17</span>

Cały łańcuch musi być traktowany jako jedna konstrukcja. Po wykonaniu jednej
prawdziwej gałęzi interpreter nie może wykonać kolejnego `ELIF` ani `ELSE`.

## 4.6. Walidacja wcięć

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">tabulacja, wielokrotność 4 i dokładne +4 od instrukcji otwierającej blok wykonane 2026-09-18</span>

Po przejściu na 4 spacje interpreter ma wymagać poprawnych poziomów wcięcia.
Niepoprawne wcięcie nie może być cicho akceptowane.

## 4.7. Nieznane instrukcje

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">nieznana instrukcja zgłasza błąd interpretera wykonane 2026-09-17</span>

```nyota
PRNIT "Hello"
```

musi zgłosić błąd interpretera zamiast zostać cicho zignorowane.

## 4.8. BEGIN / END

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">brak BEGIN/END, END przed BEGIN, druga para i kod poza blokiem są błędem wykonane 2026-09-17</span>

Jeżeli specyfikacja wymaga `BEGIN ... END`, interpreter musi to egzekwować.
Brak `BEGIN` albo `END` nie powinien powodować automatycznego wykonania całego pliku.

## 4.9. Niejawne konwersje w `+`

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">`4 + "8"` i `"8" + 4` są błędem typu; STRING + STRING oraz INTEGER + INTEGER działają wykonane 2026-09-17</span>

```nyota
4 + "8"
"8" + 4
```

nie mogą prowadzić do dwóch różnych automatycznie zgadywanych operacji.
Oba przypadki powinny wymagać jawnej konwersji.

## 4.10. FLOAT

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">FLOAT: fixed-point 3 miejsca, ucinanie nadmiaru cyfr, liczby ujemne i przepełnienia domknięte 2026-09-18</span>

Do zweryfikowania i dopracowania w v0.5:

- parser literałów `FLOAT`,
- działania,
- precyzja,
- `FLT()`,
- współpraca z `INTEGER` bez ukrytej promocji,
- dzielenie,
- liczby ujemne.

Zachowanie `=N=` względem `FLOAT` należy do APPROVED AFTER CORE.

## 4.11. Dzielenie przez zero

Dzielenie przez zero i `MOD 0` muszą generować jawny błąd interpretera.

## 4.12. RETURN

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">RETURN z wartością w FUNCTION, zakaz w PROCEDURE, błąd braku RETURN wykonane 2026-09-17</span>  
<span style="color: #006A4E;">pre-scan RETURN + blokada typu zwracanego podczas wykonania wykonane 2026-09-18</span>

Do ustalenia i przetestowania:

- `RETURN` w `FUNCTION`,
- znaczenie lub zakaz `RETURN` w `PROCEDURE`,
- typ zwracanej wartości,
- brak `RETURN`,
- wiele ścieżek zwrotu.

---

# 5. Zagrożenia

## 5.1. Rozjazd dokumentacji z interpreterem

Dokumentacja nie może opisywać konstrukcji jako działającej, jeśli interpreter
jej nie wykonuje albo wykonuje tylko częściowo.

Proponowane statusy:

```text
DZIALA
CZESCIOWO
DO NAPRAWY
DO WDROZENIA
PLAN
```

## 5.2. Zbyt szybkie rozszerzanie języka

v0.5 powinna przede wszystkim uporządkować parser, typy, zakresy, funkcje,
procedury i obsługę błędów. Zatwierdzony pomysł nie wchodzi do v0.5
automatycznie. Obowiązuje podział z działu 2.

## 5.3. Zbyt duży rdzeń interpretera

Należy rozważyć docelowy podział:

```text
rdzen jezyka
biblioteka standardowa
moduly AyoOS
moduly opcjonalne
```

## 5.4. Ciche błędy

Lepszy jest jawny błąd niż pozornie poprawny wynik. Dotyczy to szczególnie:
niezgodności typów, błędnych wcięć, dzielenia przez zero, nieznanych instrukcji
i przekraczania limitów.

## 5.5. Stałe limity

Trzeba rozróżnić limit języka, limit bieżącej implementacji i limit AyoOS.
Przekroczenie limitu ma kończyć się czytelnym błędem.

## 5.6. Własne konstrukcje bez dokładnej semantyki

Każda własna konstrukcja powinna mieć formalny zapis składni, typy,
przypadki brzegowe, przykłady i testy regresyjne.

---

# 6. Nowe

Ten dział zawiera elementy, których obecnie w Nyocie nie ma.

Status wdrożenia jest podany przy każdym punkcie. `WHILE`, `CONTINUE` i `STEP`
należą do BLOCKS v0.5. Pozostałe nowości z tego działu nie blokują wydania.

## 6.1. Operator `=N=` — porównanie z kontrolowaną precyzją

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">=N= ucina do N miejsc, ten sam typ INTEGER/FLOAT wykonane 2026-09-17</span>

`=N=` jest autorskim operatorem Nyoty.

Jest świadomym, widocznym wyjątkiem od zwykłego `=`, a nie ukrytą promocją
typów. Zwykłe `=` nadal wymaga tego samego typu.

Porównuje dwie wartości liczbowe **tego samego typu** po **ucięciu**, a nie
zaokrągleniu, do `N` miejsc po przecinku.

```nyota
IF 2.5678 =2= 2.5611:
    PRINT "Rowne"
```

Porównanie:

```text
2.5678 -> 2.56
2.5611 -> 2.56
```

Wynik: `TRUE`.

Natomiast:

```nyota
2.5699 =2= 2.5700
```

porównuje `2.56` z `2.57`, więc daje `FALSE`.

Forma ogólna:

```text
A =N= B
```

Przykłady:

```nyota
a =0= b
a =1= b
a =2= b
a =3= b
```

Oba operandy muszą być liczbowe i mieć **ten sam typ**.

```nyota
"2.56" =2= 2.56      # BŁĄD TYPU
5 =2= 5.0            # BŁĄD TYPU
FLT(5) =2= 5.0       # OK, oba FLOAT
```

Spacje wokół operatora są dozwolone. Spacje **wewnątrz** tokenu nie są
częścią operatora. Tokenizacja jest zapisana w aneksie C.

```nyota
a=2=b
a =2= b
```

oznaczają tę samą konstrukcję. Natomiast:

```nyota
a = 2 = b
```

nie jest operatorem `=N=`. To dwa zwykłe `=` rozdzielone literałem `2`.

`N` jest literałem całkowitym nieujemnym, nie zmienną. Dopuszczalny zakres
`N` zostaje domknięty przy wdrażaniu `=N=`.

Lexer v0.5 ma rozpoznawać token `=N=` i zgłaszać błąd, że operator nie jest
dostępny w v0.5. Dzięki temu późniejsze wdrożenie nie zmieni tokenizacji
istniejącego kodu.

`=N=` jest po `MARK` drugim wyraźnie autorskim elementem projektu Nyoty.

## 6.2. SORT

**Status wdrożenia:** APPROVED AFTER CORE  
**Wybór algorytmu w składni:** wykonane 2026-09-18  
<span style="color: #006A4E;">SORT lista, ASC/DESC/REVERSE oraz AUTO/BUBBLE/INSERT/SELECT/MERGE/QUICK/HEAP/SHELL/COUNTING wykonane</span>

Planowana instrukcja sortowania. `SORT` zmienia kolekcję w miejscu.

Powierzchnia języka dla użytkownika:

```nyota
SORT lista
SORT lista, DESC
```

`ASC` jest zachowaniem domyślnym i nie musi być zapisywane. Algorytm użyty
przez interpreter jest szczegółem implementacji.

Zasady robocze:

- brak kierunku oznacza sortowanie rosnące,
- `DESC` — sortowanie malejące,
- dla `INTEGER` i `FLOAT` sortowanie jest liczbowe,
- dla `STRING` sortowanie jest alfabetyczne,
- dla `DATE` i `TIME` sortowanie jest chronologiczne, po wdrożeniu tych typów,
- lista typów mieszanych nie powinna być automatycznie konwertowana do sortowania.

Jawne wskazanie algorytmu:

```nyota
SORT lista, BUBBLE
SORT lista, QUICK, DESC
```

ma sens głównie dydaktyczny. Nie blokuje podstawowego `SORT` i nie należy
do v0.5 ani do pierwszej fali po rdzeniu.

Kandydaci trybu dydaktycznego, projektowani osobno:

```text
BUBBLE
INSERT
SELECT
MERGE
QUICK
HEAP
SHELL
COUNTING
```

## 6.3. FILE

**Status wdrożenia:** wykonane w rdzeniu i hoście POSIX 2026-09-18

Wysokopoziomowe FILE działa przez `NyotaHost`. Semantyka obejmuje
FILE_WRITE/APPEND/DELETE/COPY/MOVE oraz FILE_READ/EXISTS/SIZE.
Host AyoOS wymaga jeszcze podłączenia callbacków do AyoAPI/VFS.

## 6.4. DIR / LS

**Status wdrożenia:** wykonane w rdzeniu i hoście POSIX 2026-09-18

DIR_CREATE/DELETE/COPY/MOVE oraz DIR_EXISTS/DIR_LIST działają. `LS()` jest
aliasem funkcji `DIR_LIST()` i zwraca LIST nazw, a nie wypisuje katalogu.

## 6.5. NYASM / ASM

**Status wdrożenia:** bezpieczna VM wykonana 2026-09-18; assembler natywny FUTURE

Działa pierwszy etap: R0–R3, jawne INPUT/OUTPUT i instrukcje
MOV/ADD/SUB/MUL/DIV/MOD/STORE. VM nie ma syscalli, skoków do Nyoty ani
niekontrolowanego dostępu do pamięci. `ASM` jest aliasem `NYASM`.

Natywny assembler procesora pozostaje osobnym kierunkiem FUTURE, ponieważ
nie ma jeszcze zamkniętej specyfikacji ABI i bezpieczeństwa.

## 6.6. WHILE

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">WHILE z warunkiem BOOLEAN, zero iteracji, EXIT/CONTINUE wykonane 2026-09-17</span>

`WHILE` jest pętlą z warunkiem sprawdzanym **przed** każdą iteracją.

```nyota
VAR x := 0

WHILE x < 10:
    PRINT x
    x := x + 1
```

Zasady:

- blok może wykonać się zero razy,
- warunek musi dawać wartość `BOOLEAN`,
- `INTEGER`, `FLOAT`, `STRING` itp. nie są automatycznie konwertowane na `BOOLEAN`,
- `EXIT` przerywa najbliższą aktywną pętlę.

Przykład jawnej konwersji:

```nyota
WHILE BOOL(x):
    ...
```

## 6.7. CONTINUE

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">CONTINUE w FOR/WHILE/REPEAT/DO; poza pętlą błąd wykonane 2026-09-17</span>

`CONTINUE` pomija pozostałą część bieżącej iteracji najbliższej pętli i przechodzi do następnej iteracji.

```nyota
FOR i := 1 TO 10:
    IF i = 5:
        CONTINUE
    PRINT i
```

`CONTINUE` ma działać w pętlach `FOR`, `WHILE`, `REPEAT ... UNTIL` i `DO`.

## 6.8. STEP dla FOR

**Status wdrożenia:** BLOCKS v0.5  
<span style="color: #006A4E;">FOR ... STEP n, w tym krok ujemny; STEP 0 to błąd wykonane 2026-09-17</span>

`FOR` otrzymuje możliwość jawnego określenia kroku.

```nyota
FOR i := 0 TO 100 STEP 5:
    PRINT i
```

Liczenie w dół:

```nyota
FOR i := 10 TO 1 STEP -1:
    PRINT i
```

Brak `STEP` oznacza zachowanie dotychczasowego kroku domyślnego.

## 6.9. TUPLE — niemutowalna sekwencja

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">literał, indeksowanie, LEN, IN, FOR...IN oraz konwersje LIST/TUPLE wykonane 2026-09-17</span>

`TUPLE` jest uporządkowaną, niemutowalną sekwencją. Może zawierać wartości różnych typów.

```nyota
VAR a := ()
VAR b := (5,)
VAR c := (10, "Ayo", TRUE)
```

Przecinek odróżnia jednoelementową `TUPLE` od zwykłego nawiasu grupującego.

```nyota
(5)     # INTEGER w nawiasie
(5,)    # TUPLE z jednym elementem
```

Dostęp i iteracja:

```nyota
PRINT c[0]
PRINT LEN(c)
FOR x IN c:
    PRINT x
```

Operator `IN` sprawdza obecność wartości z zachowaniem ścisłej tożsamości typu.

`TUPLE` jest niemutowalne. Niedozwolone są przypisanie do indeksu oraz instrukcje mutujące `APPEND`, `REMOVE`, `EXTEND`, `CLEAR`, `REVERSE` i `SORT`.

Konwersje tworzą nowy kontener:

```nyota
VAR lista := [1, 2, 3]
VAR t := TUPLE(lista)
VAR kopia := LIST(t)
```

Zmiana `lista` lub `kopia` nie zmienia długości ani układu `t`.

## 6.10. Rozszerzenia LIST

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">pełna zatwierdzona powierzchnia LIST: literały i zagnieżdżenia, indeksowanie, `+` `-` `><`, IN/LEN, APPEND/EXTEND/REMOVE/CLEAR/REVERSE/SORT, FOR...IN wykonane 2026-09-17</span>

Operatory `+`, `-` i `><` zwracają nową listę. Instrukcje `APPEND`, `EXTEND`, `REMOVE`,
`CLEAR`, `REVERSE` i `SORT` zmieniają istniejącą listę w miejscu.

### Łączenie list operatorem `+`

Operator `+` łączy dwie lub więcej list i zachowuje duplikaty.

```nyota
VAR a := [1, 2]
VAR b := [3, 4]
VAR c := [5, 6]
VAR wynik := a + b + c
```

Wynik:

```text
[1, 2, 3, 4, 5, 6]
```

Duplikaty pozostają:

```nyota
[1, 2, 3] + [2, 3, 4]
```

wynik:

```text
[1, 2, 3, 2, 3, 4]
```

### Odejmowanie list operatorem `-`

Dla każdego elementu prawej listy usuwane jest **jedno pierwsze pasujące wystąpienie** z lewej listy.

```nyota
[1, 1, 2, 3] - [1]
```

wynik:

```text
[1, 2, 3]
```

```nyota
[1, 1, 2, 3] - [1, 1]
```

wynik:

```text
[2, 3]
```

Przykład:

```nyota
VAR a := [1, 5, 12, 16]
VAR b := [2, 5, 11, 16]
VAR wynik := a - b
```

wynik:

```text
[1, 12]
```

### EXTEND

`EXTEND` rozszerza pierwszą listę o elementy z drugiej listy, ale dodaje tylko wartości, których w pierwszej liście jeszcze nie ma.

```nyota
VAR a := [1, 5, 12]
VAR b := [5, 16, 20]
EXTEND a, b
```

Po wykonaniu:

```text
[1, 5, 12, 16, 20]
```

### Ścisłe porównywanie elementów

W operacjach na listach typ jest częścią tożsamości wartości.

```text
5 <> "5"
5 <> 5.0
5 <> TRUE
```

Nie wykonuje się ukrytych konwersji podczas `EXTEND`, odejmowania, usuwania ani innych operacji porównujących elementy list.

### REMOVE po indeksie i po wartości

Usunięcie po indeksie:

```nyota
REMOVE lista[index]
```

Usunięcie pierwszego wystąpienia wartości:

```nyota
REMOVE lista, wartosc
```

Usunięcie wszystkich wystąpień wartości:

```nyota
REMOVE lista, wartosc, ALL
```

Przykład:

```nyota
VAR a := [5, "5", 5, 5.0]
REMOVE a, 5, ALL
```

wynik:

```text
["5", 5.0]
```

### CLEAR

`CLEAR` usuwa wszystkie elementy listy, ale zmienna nadal pozostaje typu `LIST`.

```nyota
VAR lista := [1, 5, "Ayo", TRUE]
CLEAR lista
```

Po wykonaniu:

```text
[]
```

`LEN(lista)` zwraca wtedy `0`.

### Różnica symetryczna `><`

Operator `><` zwraca elementy występujące tylko po jednej stronie. Dla `LIST`
uwzględniana jest liczba wystąpień, a porównanie pozostaje ścisłe typowo.

```nyota
[1, 5, 12, 16] >< [2, 5, 11, 16]
```

wynik:

```text
[1, 12, 2, 11]
```

Przykład z duplikatami:

```nyota
[1, 1, 2] >< [1, 3]
```

wynik:

```text
[1, 2, 3]
```

### REVERSE

`REVERSE` odwraca aktualną kolejność elementów bez sortowania.

```nyota
REVERSE lista
```

### FOR ... IN LIST

`FOR ... IN` przechodzi po elementach listy w ich bieżącej kolejności. Wyrażenie
listy jest obliczane raz przy wejściu do pętli. Zmienna iteratora jest zmienną
sterującą pętli i przy liście mieszanej przyjmuje typ aktualnego elementu.

```nyota
FOR element IN lista:
    PRINT element
```

Indeksowanie LIST jest ścisłe: indeks musi być `INTEGER`. `"1"`, `1.0` ani
`TRUE` nie są automatycznie zamieniane na indeks całkowity.

Bieżący interpreter ma limit implementacyjny `64` elementów jednej LIST.
Limit ten wynika z obecnej statycznej implementacji i nie jest deklarowanym
limitem semantycznym języka.

## 6.11. Losowe listy liczbowe

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">RANDINT i RANDFLT, count do 64 i precyzja RANDFLT 0..3 wykonane 2026-09-17</span>

### RANDINT()

`RANDINT()` tworzy listę losowych wartości `INTEGER`.

```nyota
VAR lista := RANDINT(10, 1, 100)
```

Argumenty oznaczają kolejno:

```text
liczba elementow, minimum, maksimum
```

### RANDFLT()

`RANDFLT()` tworzy listę losowych wartości `FLOAT`.

```nyota
VAR lista := RANDFLT(10, 1, 100)
```

Domyślna precyzja wynosi **2 miejsca po przecinku**.

Można podać ją jawnie jako czwarty argument:

```nyota
RANDFLT(10, 1, 100, 0)
RANDFLT(10, 1, 100, 1)
RANDFLT(10, 1, 100, 2)
RANDFLT(10, 1, 100, 3)
```

Dozwolona precyzja jest ograniczona do zakresu:

```text
0..3
```

Wartość spoza tego zakresu ma powodować błąd interpretera.

## 6.12. Rozszerzenia MARK

**Status wdrożenia:** APPROVED AFTER CORE — WDROŻONE  
<span style="color: #006A4E;">REKEY, KEY/VALUE/VALUES, IN/FOR IN, algebra + - ><, EXTEND/CLEAR/REVERSE/SORT, MLIST/MEXTEND/MINSERT/MDROP i statystyki wykonane 2026-09-17</span>

`MARK` jest autorską strukturą Nyoty łączącą klucze z uporządkowanymi
kolumnami wartości. W v0.5 ma być traktowany jako lekka struktura tabelowa,
a nie tylko prosty słownik.

Przykład:

```nyota
VAR kraje := {3| "Polska"  | "Warszawa" | "Wisla",
                 "Niemcy" | "Berlin"   | "Ren" }
```

Liczba przed `|` oznacza łączną liczbę pól w wierszu: klucz + kolumny wartości.
W operacjach kolumnowych indeksowane są wyłącznie kolumny wartości, od `0`.
Klucz pozostaje osobną częścią `MARK`.

### Odczyt i modyfikacja wartości

Odczyt po kluczu:

```nyota
kraje["Polska"][0]
```

Zmiana pojedynczej wartości:

```nyota
kraje["Polska"][0] := "Krakow"
```

Zastąpienie całego wiersza wartości:

```nyota
kraje["Polska"] := ["Krakow", "Wisla"]
```

Dodanie nowego wpisu używa tej samej składni:

```nyota
kraje["Francja"] := ["Paryz", "Sekwana"]
```

Usunięcie wpisu:

```nyota
DELETE kraje["Niemcy"]
```

### REKEY

`REKEY` zmienia sam klucz istniejącego wpisu i zachowuje jego wartości oraz
pozycję w bieżącej kolejności MARK-a.

```nyota
REKEY kraje, "Polska", "Polska_PL"
```

Nowy klucz nie może kolidować z już istniejącym kluczem.

### Dostęp według numeru wiersza

`KEY()` zwraca klucz o wskazanym indeksie:

```nyota
PRINT KEY(kraje, 2)
```

`VALUE()` zwraca pojedynczą wartość na podstawie indeksu wiersza i kolumny:

```nyota
PRINT VALUE(kraje, 2, 1)
```

`VALUES()` pozwala pobrać kilka kolumn lub zakres kolumn z jednego wiersza:

```nyota
VALUES(kraje, 2, 2, 7)
VALUES(kraje, 2, 2:7)
VALUES(kraje, 2, 0, 2:5, 8)
```

Zakres `2:7` jest domknięty: obejmuje indeksy od `2` do `7` włącznie.

### LEN i MINFO

`LEN(mark)` zwraca liczbę wierszy/wpisów.

```nyota
PRINT LEN(dane)
```

`MINFO()` zwraca dwuelementową `LIST`:

```nyota
PRINT MINFO(dane)
```

wynik ma postać:

```text
[liczba_wierszy, liczba_kolumn_wartosci]
```

Klucz nie jest liczony jako kolumna wartości.

### CLEAR

`CLEAR` usuwa wszystkie wpisy, ale zmienna pozostaje typu `MARK` i zachowuje
definicję swojej struktury kolumn.

```nyota
CLEAR dane
```

### IN

Dla `MARK` operator `IN` sprawdza istnienie **klucza**.

```nyota
IF "Polska" IN kraje:
    PRINT "Jest"
```

Tożsamość klucza jest ścisła typowo, więc `5`, `"5"` i `TRUE` są różnymi kluczami.

### Iteracja `FOR ... IN`

Iteracja po `MARK` przechodzi domyślnie po kluczach w aktualnej kolejności.

```nyota
FOR klucz IN dane:
    PRINT klucz
```

Dostęp do wartości może odbywać się przez klucz:

```nyota
FOR klucz IN dane:
    PRINT klucz, " ", dane[klucz][0]
```

### Łączenie `MARK` operatorem `+`

```nyota
VAR c := a + b
VAR d := a + b + c
```

Łączone MARK-i muszą mieć zgodną liczbę kolumn wartości. Kolejność wpisów jest
zachowywana: najpierw wpisy z lewego, potem z prawego operandu. Konflikt
tego samego klucza przy `+` jest błędem.

### EXTEND dla MARK

```nyota
EXTEND a, b
```

Dodaje do `a` wyłącznie wpisy z `b`, których kluczy jeszcze w `a` nie ma.
Istniejące klucze są pomijane, a nie nadpisywane. Struktury muszą mieć zgodną
liczbę kolumn wartości.

### Odejmowanie `MARK` operatorem `-`

```nyota
VAR c := a - b
```

Z lewego MARK-a usuwane są wpisy, których klucze występują w prawym MARK-u.
Klucze nieobecne po lewej nie powodują błędu.

### Różnica symetryczna `><`

```nyota
VAR c := a >< b
```

Wynik zawiera wpisy, których klucze występują tylko w jednym z dwóch MARK-ów.
Porównanie kluczy jest ścisłe typowo.

### Sortowanie MARK

Sortowanie po kluczach:

```nyota
SORT dane, KEY
SORT dane, KEY, DESC
```

Sortowanie po konkretnej kolumnie wartości:

```nyota
SORT dane, VALUE 0
SORT dane, VALUE 2, DESC
```

`STRING` jest sortowany alfabetycznie, `INTEGER` i `FLOAT` liczbowo. Sortowana
kolumna lub zbiór kluczy musi zawierać wartości wzajemnie porównywalnych typów.

`REVERSE` nie sortuje, a jedynie odwraca aktualną kolejność wpisów:

```nyota
REVERSE dane
```

### Funkcje statystyczne dla kolumn

Podstawowy zestaw operacji kolumnowych:

```nyota
SUM(dane, 0)
AVG(dane, 0)
MED(dane, 0)
MIN(dane, 0)
MAX(dane, 0)
MODE(dane, 0)
MODECOUNT(dane, 0)
COUNT(dane, 0, 5)
```

Znaczenie:

```text
SUM(mark, column)             suma kolumny
AVG(mark, column)             srednia arytmetyczna
MED(mark, column)             mediana
MIN(mark, column)             najmniejsza wartosc
MAX(mark, column)             najwieksza wartosc
MODE(mark, column)            najczesciej wystepujaca wartosc
MODECOUNT(mark, column)       liczba wystapien najczestszej wartosci
COUNT(mark, column, value)    liczba wystapien wskazanej wartosci
```

Dla `SUM`, `AVG`, `MED`, `MIN` i `MAX` wskazana kolumna musi zawierać wartości
`INTEGER` i/lub `FLOAT`. `COUNT`, `MODE` i `MODECOUNT` porównują wartości bez
niejawnych konwersji typów.

Przykład:

```nyota
PRINT COUNT(dane, 0, 8), " ", MODE(dane, 0), " ", MODECOUNT(dane, 0)
```

Dla kolumny `[5, 8, 5, 12, 5, 8]` wynik odpowiada wartościom `2 5 3`.

Przy remisie kilku dominant `MODE()` zwraca tę, która występuje najwcześniej
w aktualnej kolejności MARK-a. `MODECOUNT()` zwraca liczbę jej wystąpień.

### MEXTEND — dodawanie kolumn

`MEXTEND` dopisuje nowe kolumny wartości na końcu MARK-a.

Jedna wartość domyślna może wypełnić wszystkie nowe kolumny:

```nyota
MEXTEND dane, 3, 0
MEXTEND dane, 3, "Pusto"
```

Pierwszy zapis tworzy trzy nowe kolumny wypełnione `0`, drugi trzy kolumny
wypełnione napisem `"Pusto"`.

Można również podać oddzielną wartość dla każdej nowej kolumny:

```nyota
MEXTEND dane, 3, 0, "Pusto", TRUE
```

Wtedy nowe kolumny otrzymują odpowiednio wartości i typy:

```text
0          INTEGER
"Pusto"    STRING
TRUE       BOOLEAN
```

Jeśli po liczbie kolumn występuje więcej niż jedna wartość, ich liczba musi być
równa liczbie dodawanych kolumn.

### MINSERT — wstawianie kolumny

`MINSERT` wstawia nową kolumnę wartości pod wskazanym indeksem i wypełnia ją
podaną wartością.

```nyota
MINSERT dane, 2, "Pusto"
```

Po wstawieniu dotychczasowa kolumna `2` i wszystkie późniejsze przesuwają się
o jedną pozycję w prawo.

### MDROP — usuwanie kolumny

`MDROP` usuwa kolumnę wartości o wskazanym indeksie ze wszystkich wierszy MARK-a.

```nyota
MDROP dane, 2
```

Indeksowanie kolumn jest zerowe. Klucz nie jest kolumną `0` i nie może zostać
usunięty przez `MDROP`. Nieistniejący indeks kolumny powoduje błąd.

### MLIST — konwersja pionowego wycinka MARK-a do LIST

`MLIST()` zwraca wskazaną kolumnę wartości jako zwykłą `LIST`.

```nyota
MLIST(dane, 0)
MLIST(dane, 1)
MLIST(dane, 2)
```

`MLIST(dane, 2)` oznacza listę utworzoną z **trzeciej kolumny wartości**.

Specjalny selektor `KEY` zwraca listę kluczy w aktualnej kolejności:

```nyota
MLIST(dane, KEY)
```

Przykład:

```nyota
VAR dane := {4| "A" | 10 | "Jan"   | TRUE,
                "B" | 20 | "Anna"  | FALSE,
                "C" | 30 | "Piotr" | TRUE }
```

wyniki:

```text
MLIST(dane, 0)     -> [10, 20, 30]
MLIST(dane, 1)     -> ["Jan", "Anna", "Piotr"]
MLIST(dane, 2)     -> [TRUE, FALSE, TRUE]
MLIST(dane, KEY)   -> ["A", "B", "C"]
```


## 6.13. DATE — pełnoprawny typ daty

**Status wdrożenia:** APPROVED AFTER CORE (wdrożone wcześniej na prośbę)  
<span style="color: #006A4E;">literał `&lt;RRRR.MM.DD&gt;`, gregoriańskie lata przestępne, DATE±INTEGER, DATE−DATE, porównania, YEAR/MONTH/DAY/TODAY wykonane 2026-09-17</span>  
<span style="color: yellow;">SORT dat, MIN/MAX na kolumnie DATE i współpraca z MARK zaczęte 2026-09-17</span>

`DATE` jest pełnoprawnym typem Nyoty. Literal daty ma charakterystyczną,
jednoznaczną postać:

```nyota
VAR data := <2026.09.17>
```

Kanoniczny układ daty:

```text
<RRRR.MM.DD>
```

Data nie jest `STRING`. Interpreter ma walidować ją już podczas tworzenia
wartości i nie może automatycznie poprawiać nieprawidłowej daty.

Przykłady:

```nyota
VAR a := <2024.02.29>   # poprawna
VAR b := <2026.02.29>   # BŁĄD
VAR c := <2026.02.30>   # BŁĄD
VAR d := <2026.13.01>   # BŁĄD
VAR e := <2000.02.29>   # poprawna
VAR f := <2100.02.29>   # BŁĄD
```

### Lata przestępne

Nyota stosuje pełną regułę kalendarza gregoriańskiego.

Rok jest przestępny, jeżeli:

```text
dzieli sie przez 400

LUB

dzieli sie przez 4
i jednoczesnie nie dzieli sie przez 100
```

Przykłady:

```text
2024 -> przestepny
2026 -> nieprzestepny
2000 -> przestepny
2100 -> nieprzestepny
2400 -> przestepny
```

### Arytmetyka DATE

Odejmowanie dwóch dat zwraca różnicę w dniach jako `INTEGER`:

```nyota
VAR a := <2026.09.17>
VAR b := <2026.09.10>

VAR dni := a - b     # 7
VAR odwrotnie := b - a   # -7
```

Dodanie lub odjęcie `INTEGER` przesuwa datę o wskazaną liczbę dni:

```nyota
VAR data := <2026.09.17>

VAR jutro := data + 1
VAR tydzien_wczesniej := data - 7
```

Dozwolone:

```text
DATE - DATE -> INTEGER
DATE + INTEGER -> DATE
DATE - INTEGER -> DATE
```

Niedozwolone:

```text
DATE + DATE
DATE * DATE
DATE / DATE
```

### Porównania i sortowanie DATE

Daty można porównywać chronologicznie:

```nyota
<2026.09.17> < <2027.01.01>
<2026.09.17> > <2025.12.31>
<2026.09.17> = <2026.09.17>
```

`LIST` zawierająca daty może być sortowana:

```nyota
SORT daty
SORT daty, DESC
```

Kolumna `MARK` typu `DATE` również może być sortowana:

```nyota
SORT dane, VALUE 2
SORT dane, VALUE 2, DESC
```

Dla kolumn `DATE` funkcje `MIN()` i `MAX()` działają chronologicznie.
`SUM()` i `AVG()` dla dat są niedozwolone.

### Funkcje DATE

Podstawowy zestaw funkcji:

```nyota
TODAY()
YEAR(data)
MONTH(data)
DAY(data)
```

Znaczenie:

```text
TODAY()      aktualna data systemowa jako DATE
YEAR(data)   rok jako INTEGER
MONTH(data)  miesiac jako INTEGER
DAY(data)    dzien miesiaca jako INTEGER
```

Przykład:

```nyota
VAR dzis := TODAY()

PRINT YEAR(dzis), ".", MONTH(dzis), ".", DAY(dzis)
```

`DATE` ma współpracować z `LIST`, `MARK`, `SORT`, `MIN`, `MAX`, `VALUE`,
`VALUES` i `MLIST` bez konwersji do tekstu.

## 6.14. TIME — pełnoprawny typ czasu

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">TIME(), TIME(HH.MM.SS), HOUR/MINUTE/SECOND, przesunięcia H/M/S, TIME-TIME, porównania i SORT wykonane 2026-09-17</span>

Kierunek składni pozostaje zatwierdzony:

```nyota
TIME(14.20.20)
TIME(14.20.20) + M15
```

Po rozszerzeniu projektu o arytmetykę godzin, minut i sekund `TIME` nie jest
zwykłą `LIST`, lecz pełnoprawnym typem Nyoty.

Aktualny czas systemowy:

```nyota
VAR teraz := TIME()
```

Jawna wartość czasu:

```nyota
VAR start := TIME(14.20.20)
```

Kanoniczny układ:

```text
TIME(HH.MM.SS)
```

Dozwolone zakresy:

```text
HH = 0..23
MM = 0..59
SS = 0..59
```

Interpreter musi odrzucać każdą wartość spoza zakresu oraz niepoprawną liczbę
elementów.

```nyota
TIME(23.59.59)   # poprawna
TIME(24.00.00)   # BŁĄD
TIME(19.65.01)   # BŁĄD
TIME(14.20.60)   # BŁĄD
```

Forma z przecinkami nie jest alternatywną składnią czasu:

```nyota
TIME(14, 15, 55, 5)   # BŁĄD
TIME(25, 45, 10)      # BŁĄD
TIME(19, 65, 01)      # BŁĄD
```

### Arytmetyka TIME

Do przesuwania czasu służą jawne jednostki:

```text
Hn   n godzin
Mn   n minut
Sn   n sekund
```

Przykłady:

```nyota
TIME(14.20.20) + H2    # TIME(16.20.20)
TIME(14.20.20) + M15   # TIME(14.35.20)
TIME(14.20.20) + S40   # TIME(14.21.00)

TIME(14.20.20) - H1    # TIME(13.20.20)
TIME(14.20.20) - M30   # TIME(13.50.20)
```

Przejście przez granicę doby zawija czas w zakresie jednej doby:

```nyota
TIME(23.50.00) + M20   # TIME(00.10.00)
TIME(00.10.00) - M20   # TIME(23.50.00)
```

Różnica `TIME - TIME` zwraca `INTEGER` wyrażony w sekundach. Jest to różnica
prostych wartości pory dnia bez automatycznego wybierania krótszej drogi przez północ.

```nyota
TIME(14.30.00) - TIME(13.00.00)   # 5400
TIME(01.00.00) - TIME(23.00.00)   # -79200
```

`DATETIME` pozostaje osobnym kierunkiem FUTURE i nie zmienia tej reguły.

### Porównania i sortowanie TIME

Wartości `TIME` można porównywać jako pory dnia:

```nyota
TIME(14.30.00) < TIME(16.00.00)
TIME(20.00.00) > TIME(08.00.00)
TIME(12.00.00) = TIME(12.00.00)
```

`LIST` wartości `TIME` może być sortowana:

```nyota
SORT godziny
SORT godziny, DESC
```

Kolumna `MARK` typu `TIME` również podlega sortowaniu:

```nyota
SORT plan, VALUE 0
SORT plan, VALUE 0, DESC
```

### Funkcje TIME

Zestaw funkcji jest analogiczny do funkcji `DATE`:

```nyota
TIME()
HOUR(czas)
MINUTE(czas)
SECOND(czas)
```

Znaczenie:

```text
TIME()         aktualny czas systemowy jako TIME
HOUR(czas)     godzina jako INTEGER
MINUTE(czas)   minuta jako INTEGER
SECOND(czas)   sekunda jako INTEGER
```

Przykład:

```nyota
VAR teraz := TIME()

PRINT HOUR(teraz), ":", MINUTE(teraz), ":", SECOND(teraz)
```

`TIME` ma współpracować z `LIST`, `MARK`, `SORT`, `VALUE`, `VALUES` i `MLIST`
jako rzeczywisty typ, bez zamiany na `[HH, MM, SS]`.

## 6.15. TABLE — nazwana kontrolka prezentacji danych

**Status wdrożenia:** FUTURE (pierwszy etap wykonany wcześniej na prośbę)  
<span style="color: #006A4E;">nazwana kontrolka, układ kolumn, font/rozmiar, kolor tekstu, LIST/TUPLE/MARK i TABLE_DATA wykonane 2026-09-17</span>

`TABLE` **nie jest typem zmiennej**. Jest kontrolką prezentacyjną działającą na
warstwie graficznej Nyoty. Dane pozostają w istniejących typach takich jak
`LIST`, `TUPLE` i `MARK`.

Każda tabela ma obowiązkową **nazwę logiczną**, dzięki której program może
odwoływać się do konkretnej kontrolki, gdy tabel jest wiele. Nazwa należy do
osobnej przestrzeni kontrolek TABLE i nie jest zmienną Nyoty.

Podstawowa składnia:

```nyota
TABLE kraje_view, 20, 60, 700, 300, 3, [180, 260, 260], "SYSTEM", 14, 220, 220, 220
```

Parametry oznaczają kolejno:

```text
nazwa tabeli
x, y
szerokosc, wysokosc
liczba kolumn
LIST/TUPLE szerokosci poszczegolnych kolumn
nazwa czcionki
rozmiar czcionki
R, G, B koloru tekstu
opcjonalne zrodlo danych
```

W pierwszym etapie suma szerokości kolumn musi być dokładnie równa szerokości
kontrolki. Każda szerokość jest dodatnim `INTEGER`, a liczba pozycji w liście
szerokości musi odpowiadać liczbie kolumn.

Źródło można podać od razu:

```nyota
TABLE kraje_view, 20, 60, 700, 300, 3, [180, 260, 260], "SYSTEM", 14, 220, 220, 220, kraje
```

albo później zmienić je przez nazwę tabeli:

```nyota
TABLE_DATA kraje_view, inne_dane
```

Źródło jest nazwą istniejącej zmiennej `LIST`, `TUPLE` albo `MARK`; `TABLE` nie
kopiuje danych do nowego typu.

Zasady prezentacji pierwszej wersji:

```text
MARK             kolumna 0 = klucz, dalej kolumny wartosci
LIST/TUPLE       jedna kolumna -> elementy jako kolejne wiersze
LIST/TUPLE       wiele kolumn -> elementy musza byc wierszami LIST/TUPLE
brak zrodla       pusta kontrolka z ukladem kolumn
```

Dla `MARK` liczba kolumn TABLE musi odpowiadać: `klucz + kolumny wartości`.
Dla wielokolumnowych `LIST/TUPLE` każdy wiersz musi mieć dokładnie tyle pól,
ile zadeklarowano kolumn.

Ponowne wykonanie `TABLE` z tą samą nazwą aktualizuje tę samą logiczną kontrolkę,
a nie tworzy drugiej o nierozróżnialnym identyfikatorze.

Nazwa fontu jest częścią kontraktu kontrolki. Host może użyć fontu zastępczego,
jeżeli nie posiada wskazanej czcionki; referencyjny host POSIX nadal używa
wbudowanego fontu bitmapowego i skaluje go do żądanego rozmiaru.

Na tym etapie TABLE nie zapewnia jeszcze nagłówków, edycji komórek, zaznaczania,
przewijania ani sortowania kliknięciem. To są późniejsze możliwości GUI, nie
warunek istnienia podstawowej kontrolki prezentacyjnej.

## 6.14a. BUTTON — nazwana kontrolka GUI

**Status wdrożenia:** FUTURE (pierwszy etap wykonany wcześniej na prośbę)  
<span style="color: #006A4E;">nazwa logiczna, geometria, tekst, font/rozmiar, kolory tekstu/tła, renderowanie i BUTTON_CLICKED() na hoście POSIX wykonane 2026-09-17</span>

`BUTTON` nie jest typem zmiennej. Nazwa identyfikuje kontrolkę, tak aby program
mógł utrzymywać wiele przycisków jednocześnie.

```nyota
BUTTON zapisz, 40, 40, 160, 48, "Zapisz", "SYSTEM", 14, 255, 255, 255, 40, 110, 180
IF BUTTON_CLICKED(zapisz):
    PRINT "klik"
```

Pierwszy etap definiuje kontrolkę i semantykę kliknięcia. Rozbudowany wspólny
system zdarzeń GUI, focus, disabled/hover, tab-order i callbacki pozostają częścią
przyszłego standardu GUI.

## 6.16. Kierunki FUTURE

**Status wdrożenia:** wymagają osobnej specyfikacji przed kodem

Ten dział nie definiuje składni ani kontraktu wykonania. Zgodnie z zasadą tego samego dokumentu interpreter nie może zgadywać semantyki. Nazwy poniżej są kierunkami projektowymi, a nie gotowymi instrukcjami do mechanicznego dodania.

Poniższe elementy można projektować wcześniej. Nie należą do v0.5 i nie
należą do pierwszej fali po rdzeniu:

```text
relacyjny MARK
DATETIME
standard GUI Nyoty
sprite'y
audio
assembler natywny
Windows jako host Nyoty
relacyjny MARK / DATETIME / standard GUI / audio
```

---

# 7. Kierunek

Nyota powinna być przede wszystkim językiem AyoOS do:

- aplikacji użytkowych,
- narzędzi systemowych,
- automatyzacji,
- prostych i średnich programów,
- grafiki,
- prostych gier 2D,
- edukacji,
- szybkiego prototypowania.

Nie powinna próbować zastąpić C lub Zig w najniższych warstwach systemu.

Priorytet v0.5 to lista BLOCKS z działu 2, nie pełna lista zatwierdzonych
pomysłów:

```text
1. Jednoznaczna specyfikacja elementów BLOCKS, w tym ścisłego =.
2. Naprawa parsera i semantyki istniejących konstrukcji.
3. Ujednolicenie systemu typow i konwersji.
4. Pelne FUNCTION / PROCEDURE / zakresy.
5. Pelna walidacja bledow.
6. WHILE, CONTINUE, STEP.
7. Lexer rezerwuje tokeny z aneksu C.
8. Aktualizacja dokumentacji v0.5.
9. Testy regresyjne.
10. Wydanie v0.5.
11. Dopiero potem APPROVED AFTER CORE.
12. FUTURE pozostaje projektem i wymaga najpierw domknięcia semantyki; nie jest atrapą instrukcji w interpreterze.
```

Charakterystyczne elementy Nyoty opisują tożsamość języka, a nie checklistę
wydania v0.5:

```text
ścisłe =  (typ + wartość); brak == i ===
MARK
=N= jako porownanie po ucieciu do N miejsc
DATE z literalem <RRRR.MM.DD> i wbudowana arytmetyka dat
TIME(HH.MM.SS) z jednostkami H, M i S
% jako procent
MOD jako modulo
^ jako potegowanie
^^ jako pierwiastkowanie
```

---

# 8. Dokumentacja

Po ustabilizowaniu v0.5 należy określić jeden nadrzędny dokument specyfikacji.

`nyota_v05.md` jest dokumentem projektowym i przejściowym. Nie jest workiem
wszystkich zatwierdzonych pomysłów o tym samym priorytecie. Obowiązuje
podział z działu 2.

Dokumentacja nie może przedstawiać planowanej konstrukcji jako działającej.
Konstrukcja ze statusem APPROVED AFTER CORE albo FUTURE nie może być opisana
w `nyota.md` jako działająca.
Każda istotna reguła powinna mieć:

- przykład poprawny,
- przykład błędny,
- oczekiwany wynik,
- test regresyjny.

Do synchronizacji po wdrożeniu:

```text
nyota.c
nyota.md
nyota.txt
do_wdrożenia.md
pomoc AyoEdit / Tunga
przyklady .nyo
testy Nyoty
dokumentacja AyoOS
```

---

# 9. Przykłady

## 9.1. Hello World

```nyota
BEGIN
PRINT "Hello World"
END
```

## 9.2. Zmienne

```nyota
BEGIN
VAR liczba := 10
VAR tekst := "Nyota"
VAR aktywny := TRUE

PRINT liczba
PRINT tekst
PRINT aktywny
END
```

## 9.3. Warunek

```nyota
BEGIN
VAR temperatura := 25

IF temperatura > 20:
    PRINT "Cieplo"
ELSE:
    PRINT "Chlodno"

END
```

## 9.4. AND / OR / NOT

```nyota
BEGIN
VAR aktywny := TRUE
VAR admin := FALSE
VAR zablokowany := FALSE

IF aktywny AND NOT zablokowany:
    PRINT "Dostep"

IF admin OR aktywny:
    PRINT "Uzytkownik moze kontynuowac"

END
```

## 9.5. Jawna konwersja

```nyota
BEGIN
VAR a := 4
VAR b := "8"

VAR suma := a + INT(b)
VAR tekst := STR(a) + b

PRINT suma
PRINT tekst
END
```

Oczekiwane:

```text
12
48
```

## 9.6. BOOLEAN i INTEGER

```nyota
BEGIN
VAR b := TRUE
VAR a := 2

VAR c := a + INT(b)

PRINT c
END
```

Oczekiwane: `3`.

## 9.7. Procent i modulo

```nyota
BEGIN
VAR cena := 200
VAR rabat := cena % 15
VAR reszta := 10 MOD 3

PRINT rabat
PRINT reszta
END
```

## 9.8. Potęga i pierwiastek

```nyota
BEGIN
VAR potega := 2 ^ 3
VAR pierwiastek := 9 ^^ 2

PRINT potega
PRINT pierwiastek
END
```

## 9.9. Ścisłe porównanie `=`

**Status wdrożenia:** BLOCKS v0.5

```nyota
BEGIN
VAR a := 5
VAR b := 5
VAR c := 5.0

PRINT a = b          # TRUE
# PRINT a = c        # BŁĄD TYPU
PRINT FLT(a) = c     # TRUE
END
```

## 9.10. Porównanie `=N=`

**Status wdrożenia:** APPROVED AFTER CORE

```nyota
BEGIN
VAR a := 2.5678
VAR b := 2.5611

IF a =2= b:
    PRINT "Rowne do dwoch miejsc po ucieciu"
ELSE:
    PRINT "Rozne"

END
```

## 9.11. DATE

**Status wdrożenia:** APPROVED AFTER CORE  
<span style="color: #006A4E;">przykład DATE działa w interpreterze wykonane 2026-09-17</span>

```nyota
BEGIN
VAR start := <2026.09.10>
VAR koniec := <2026.09.17>

PRINT koniec - start
PRINT start + 30
PRINT YEAR(start), ".", MONTH(start), ".", DAY(start)
END
```

Oczekiwane znaczenie:

```text
7
<2026.10.10>
2026.9.10
```

## 9.12. TIME

**Status wdrożenia:** APPROVED AFTER CORE

```nyota
BEGIN
VAR start := TIME(14.20.20)
VAR pozniej := start + M15

PRINT pozniej
PRINT HOUR(pozniej), ":", MINUTE(pozniej), ":", SECOND(pozniej)
END
```

Oczekiwane znaczenie:

```text
TIME(14.35.20)
14:35:20
```

## 9.13. Docelowa kolejność dalszych przykładów

Najpierw przykłady BLOCKS v0.5:

1. `FOR` i `STEP`
2. `WHILE` i `CONTINUE`
3. `REPEAT / UNTIL`
4. `SWITCH / CASE`
5. `FUNCTION` i `PROCEDURE`
6. parametry `VAR`
7. `RECORD`
8. obsługa błędów

Po v0.5:

9. `LIST`
10. `MARK`
11. `DATE` / `TIME`
12. grafika
13. pliki i katalogi
14. małe narzędzie AyoOS
15. prosta gra lub aplikacja demonstracyjna

---

# Aneks A — decyzje zatwierdzone podczas projektowania v0.5

Na dzień 2026-09-17 ustalono:

```text
1. Jeden poziom wciecia = 4 spacje.
2. % pozostaje operatorem procentu.
3. Modulo bedzie zapisywane jako MOD.
4. ^ pozostaje potegowaniem.
5. ^^ pozostaje pierwiastkowaniem.
6. Typ zmiennej jest wnioskowany przy deklaracji i pozniej zablokowany.
7. Konwersje miedzy roznymi typami maja byc jawne.
8. INT(TRUE) = 1, INT(FALSE) = 0.
9. BOOL(0) = FALSE, BOOL(wartosc rozna od 0) = TRUE.
10. BOOL() normalizuje wartosc do TRUE/FALSE.
11. Krotkie funkcje konwersji: INT(), FLT(), STR(), BOOL().
12. = porównuje jednocześnie typ i wartość. Różne typy dają błąd, nie FALSE.
13. == i === nie istnieją i nie wejdą do Nyoty.
14. >< przestaje byc aliasem nierownosci i staje sie roznica symetryczna LIST/MARK.
15. PRINT ma przyjmowac wiele argumentow roznych typow i konwertowac je tylko do wyswietlenia.
16. WHILE sprawdza warunek przed iteracja i wymaga BOOLEAN.
17. CONTINUE pomija reszte biezacej iteracji najblizszej petli.
18. FOR otrzyma STEP, w tym krok ujemny.
19. FOR ... IN ma obslugiwac iteracje po LIST i MARK.
20. LIST + LIST laczy listy i zachowuje duplikaty.
21. LIST - LIST usuwa po jednym pierwszym pasujacym wystapieniu dla elementow prawej listy.
22. LIST >< LIST wykonuje roznice symetryczna z uwzglednieniem liczby wystapien.
23. EXTEND listy dodaje tylko wartosci jeszcze nieobecne w liscie docelowej.
24. Porownania elementow LIST sa scisle typowo: 5, "5", 5.0 i TRUE sa rozne.
25. REMOVE lista[index] usuwa po indeksie.
26. REMOVE lista, wartosc usuwa pierwsze wystapienie wartosci.
27. REMOVE lista, wartosc, ALL usuwa wszystkie wystapienia wartosci.
28. CLEAR lista oproznia liste, zachowujac typ LIST.
29. REVERSE lista odwraca kolejnosc bez sortowania.
30. RANDINT(count, min, max) tworzy LIST wartosci INTEGER.
31. RANDFLT(count, min, max [, precision]) tworzy LIST wartosci FLOAT.
32. RANDFLT ma domyslna precyzje 2, a dozwolony zakres precision to 0..3.
33. SORT listy obsluguje SORT lista oraz SORT lista, DESC; algorytm jest szczegolem implementacji.
34. MARK zachowuje scisla tozsamosc kluczy wedlug wartosci i typu.
35. KEY(mark, index) zwraca klucz wedlug pozycji; VALUE i VALUES obsluguja dostep tabelowy.
36. VALUES(mark, row, a:b) uzywa zakresu domknietego.
37. REKEY zmienia sam klucz i zachowuje pozycje wpisu.
38. CLEAR mark usuwa wszystkie wpisy, zachowujac strukture MARK.
39. Dla MARK operator IN sprawdza istnienie klucza.
40. FOR klucz IN mark iteruje po kluczach w aktualnej kolejnosci.
41. MARK + MARK laczy zgodne struktury; konflikt klucza jest bledem.
42. EXTEND mark1, mark2 dodaje tylko wpisy o nowych kluczach.
43. MARK - MARK usuwa wpisy wedlug kluczy prawego operandu.
44. MARK >< MARK zwraca wpisy o kluczach wystepujacych tylko po jednej stronie.
45. SORT mark, KEY / KEY, DESC sortuje po kluczach.
46. SORT mark, VALUE n / VALUE n, DESC sortuje po wskazanej kolumnie wartosci.
47. REVERSE mark odwraca aktualna kolejnosc wpisow bez sortowania.
48. SUM, AVG, MED, MIN, MAX dzialaja na wskazanej liczbowej kolumnie MARK.
49. COUNT(mark, col, value) liczy wystapienia wskazanej wartosci.
50. MODE(mark, col) zwraca najczestsza wartosc, MODECOUNT(mark, col) liczbe jej wystapien.
51. MEXTEND mark, N, value dodaje N kolumn na koncu i wypelnia je jedna wartoscia.
52. MEXTEND mark, N, value1...valueN pozwala nadac kazdej nowej kolumnie osobna wartosc i typ.
53. MINSERT mark, index, value wstawia nowa kolumne wartosci.
54. MDROP mark, index usuwa wskazana kolumne wartosci ze wszystkich wierszy.
55. MINFO(mark) zwraca [liczba_wierszy, liczba_kolumn_wartosci].
56. MLIST(mark, n) zwraca n-ta kolumne wartosci jako LIST; MLIST(mark, KEY) zwraca LIST kluczy.
57. DATE jest pelnoprawnym typem z literalem <RRRR.MM.DD>.
58. Literal DATE jest zawsze walidowany; niepoprawna data powoduje blad.
59. Lata przestepne DATE sa wyznaczane wedlug pelnej reguly gregorianskiej.
60. DATE - DATE zwraca roznice dni jako INTEGER.
61. DATE + INTEGER i DATE - INTEGER przesuwaja date o liczbe dni.
62. DATE mozna porownywac i sortowac w LIST oraz w kolumnach MARK.
63. Podstawowe funkcje DATE: TODAY(), YEAR(), MONTH(), DAY().
64. TIME jest pelnoprawnym typem, a nie LIST.
65. TIME() zwraca aktualny czas systemowy, TIME(HH.MM.SS) tworzy jawna wartosc czasu.
66. TIME waliduje HH=0..23, MM=0..59 i SS=0..59.
67. TIME + / - Hn, Mn, Sn przesuwa czas o godziny, minuty lub sekundy.
68. Przekroczenie granicy doby przez TIME zawija wartosc w zakresie jednej doby.
69. TIME mozna porownywac i sortowac w LIST oraz w kolumnach MARK.
70. Podstawowe funkcje TIME: HOUR(), MINUTE(), SECOND(); TIME() odczytuje czas systemowy.
71. Statusy wdrożenia: BLOCKS v0.5, APPROVED AFTER CORE, FUTURE.
72. v0.5 blokują: parser, typy, zakresy, FUNCTION/PROCEDURE, IF/ELIF/ELSE,
    wcięcia, BEGIN/END, błędy, jawne konwersje, WHILE, CONTINUE, STEP,
    ścisłe = oraz testy regresyjne.
73. Zatwierdzenie semantyki nie oznacza wdrożenia w v0.5.
74. =N= pozostaje jawnym operatorem liczbowym; oba operandy mają ten sam typ.
75. SORT dane oraz SORT dane, DESC są powierzchnią języka.
    Wybór algorytmu w składni jest FUTURE / dydaktyczny.
76. TIME(HH.MM.SS) i Hn/Mn/Sn są zatwierdzonym kierunkiem.
    Implementacja TIME czeka na TIME - TIME i decyzję o DATETIME.
77. Przed wdrożeniem =N=, DATE, TIME, jednostek H/M/S i zakresów a:b
    obowiązuje aneks C. Lexer v0.5 rozpoznaje te tokeny i odrzuca je
    jawnym błędem.
78. Relacyjny MARK, dalszy rozwój TABLE/GUI i sprite'ów, audio oraz natywny assembler pozostają FUTURE; podstawowy SPRITE/animacja, dydaktyczny SORT i bezpieczny NYASM są wdrożone.
79. Nyota jest niezależnym językiem. Tunga jest osobnym edytorem.
    AyoOS, Linux i Windows to hosty. PRINT, GRAPH, INPUT, DELAY
    należą do języka i muszą działać na każdym hoście.
```

Decyzje 14–70 pozostają w mocy jako semantyka. Ich status wdrożenia wynika
z działu 2, a nie z kolejności na tej liście.

Stara decyzja „=== nie jest potrzebne” zostaje rozszerzona: **ani `==`,
ani `===` nie wchodzą do języka**.

---

# Aneks B — zasada klasyfikacji wpisów

Każdy wpis ma dwie osie. Nie wolno ich mylić.

Oś rodzaju:

```text
ZMIANA
    Element juz istnieje w Nyocie, ale chcemy zmienic jego skladnie,
    zachowanie albo definicje.

NAPRAWA
    Element juz istnieje i jego zamierzona definicja jest znana,
    ale interpreter realizuje ja blednie lub niepelnie.

NOWE
    Elementu obecnie w Nyocie nie ma i dopiero ma zostac dodany.

ZAGROZENIE
    Ryzyko architektoniczne, projektowe albo jakosciowe, ktore moze
    utrudnic rozwoj lub prowadzic do bledow.
```

Oś wdrożenia:

```text
BLOCKS v0.5
    Bez tego v0.5 nie wychodzi.

APPROVED AFTER CORE
    Semantyka zatwierdzona. Wdrożenie po naprawie rdzenia.

FUTURE
    Kierunek zatwierdzony koncepcyjnie, nadal projektowany
    albo świadomie odłożony po v0.5.
```

`NOWE` + `APPROVED AFTER CORE` to zatwierdzona semantyka poza wydaniem.
`NOWE` + `BLOCKS v0.5` dotyczy tylko luk rdzenia: `WHILE`, `CONTINUE`, `STEP`.

---

# Aneks C — reguły tokenizacji

Ten aneks jest fragmentem specyfikacji leksera. Nie ustala się go przy
okazji implementacji kolejnego ficzera.

Zasady ogólne:

```text
1. Tokenizacja nie zgaduje semantyki typów.
2. Spacje wewnątrz tokenu są częścią definicji albo są zakazane.
3. Przy konflikcie wzorców wygrywa najdłuższe dopasowanie.
4. Lexer v0.5 rozpoznaje tokeny C.3–C.7. C.4 (DATE) ma semantykę.
   Pozostałe (C.3, C.5–C.7) odrzuca jawnym błędem zamiast nadawać
   im tymczasowe znaczenie.
```

## C.1. Przypisanie i porównanie

```text
:=     jeden token przypisania
=      jeden token ścisłego porównania
<>     jeden token nierówności
```

Skanowanie `=`:

```text
po "=" :
    jeżeli dalej cyfry i od razu "="   -> token =N=  (poza v0.5: błąd)
    jeżeli dalej "="                   -> błąd, == nie istnieje
    w przeciwnym razie                 -> token =
```

`==` i `===` są zawsze błędem. Lexer nie rozbija `==` na dwa tokeny `=`.

## C.2. Operatory słowne i `><`

```text
MOD    token operatora modulo (BLOCKS v0.5)
><     jeden token
```

W v0.5 `><` pozostaje aliasem nierówności. Nowe znaczenie (różnica
symetryczna) jest APPROVED AFTER CORE i nie zmienia postaci tokenu.

## C.3. Operator `=N=`

Token:

```text
"=" + jedna lub więcej cyfr dziesiętnych + "="
```

Bez spacji wewnątrz tokenu.

```nyota
a=2=b          # token =2=
a =2= b        # to samo
a = 2 = b      # NIE jest =N= :  =   2   =
```

`N` jest literałem całkowitym nieujemnym, nie identyfikatorem.

```nyota
a =k= b        # NIE jest =N=
```

Token `=N=` jest rozpoznawany i porównywany (ucięcie do N miejsc).

## C.4. Literał DATE

**Status:** semantyka wdrożona 2026-09-17.

Token kanoniczny:

```text
"<" + 4 cyfry + "." + 2 cyfry + "." + 2 cyfry + ">"
```

Bez spacji wewnątrz tokenu.

```nyota
<2026.09.17>     # literał DATE
< 2026.09.17 >   # NIE jest literałem DATE
```

Jeżeli wzorzec nie pasuje, `<` jest operatorem porównania:

```nyota
x < 5
x<5
```

Niepoprawna data o poprawnym kształcie, na przykład `<2026.02.30>`,
jest tokenem DATE i odpada dopiero przy walidacji kalendarza.

## C.5. TIME(HH.MM.SS)

`FLOAT` ma co najwyżej jedną kropkę. Zapis z dwiema kropkami nie jest liczbą.

```text
TIME_TRIPLE = liczby + "." + liczby + "." + liczby
```

Każda część to jedna albo dwie cyfry. Forma kanoniczna w przykładach to
dwie cyfry: `14.20.20`, `00.10.00`.

`TIME_TRIPLE` jest legalny wyłącznie jako jedyny argument `TIME(...)`.

```nyota
TIME(14.20.20)       # OK po wdrożeniu TIME
TIME(14, 20, 20)     # BŁĄD, przecinki nie tworzą czasu
14.20.20             # BŁĄD poza TIME(...)
14.20                # FLOAT
```

W v0.5 `TIME_TRIPLE` jest rozpoznawany i odrzucany jawnym błędem.

## C.6. Jednostki `Hn` / `Mn` / `Sn`

Token jednostki czasu:

```text
"H" albo "M" albo "S" + jedna lub więcej cyfr
bez spacji
```

```nyota
H2
M15
S30
```

`H 2` nie jest jednostką czasu.

Od wdrożenia `TIME` ten wzorzec nie jest identyfikatorem. Do v0.5 lexer
może rozpoznawać go i odrzucać jako konstrukcję poza v0.5, żeby później
nie zabrać nazwy zmiennej.

Użycie poza `TIME +` / `TIME -` będzie błędem składni.

## C.7. Zakres `a:b`

Token zakresu:

```text
liczba całkowita + ":" + liczba całkowita
bez spacji wewnątrz
```

Zakres jest domknięty: `2:7` obejmuje indeksy od 2 do 7 włącznie.

```nyota
VALUES(kraje, 2, 2:7)
```

`2 : 7` nie jest tym tokenem. Pętla `FOR` nadal używa `TO`, nie dwukropka.

W v0.5 token zakresu jest rozpoznawany i odrzucany jawnym błędem, jeżeli
wystąpi. Semantyka `VALUES` pozostaje APPROVED AFTER CORE.

## C.8. Kolejność zamykania leksera

```text
1. Zamknąć C.1 i C.2 razem z parserem v0.5.
2. C.4 DATE ma semantykę. C.3, C.5–C.7 nadal rezerwować jako błędy.
3. Semantykę =N=, TIME, Hn/Mn/Sn i a:b wdrażać bez zmiany reguł tokenów.
```
