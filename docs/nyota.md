# NYOTA — bieżąca specyfikacja implementacji v0.5

**Ostatnia weryfikacja całości:** 2026-09-18  
**Powiązane:** [`docs/nyota_v05.md`](../../../docs/nyota_v05.md) — plan wydania v0.5 i rozwoju po rdzeniu; ten plik jest specyfikacją dla agenta i ma opisywać tylko to, czego ma używać, z jawnym stanem interpretera.

> Cel dokumentu: ten plik ma być podawany agentowi AI jako nadrzędny kontekst przy analizie, poprawianiu i generowaniu kodu w języku Nyota.
>
> Nyota jest własnym językiem programowania, niezależnym od edytora i od systemu.
> Tunga jest osobnym projektem (edytor), nie częścią Nyoty.
> AyoOS jest pierwszym systemem, na którym Nyota działa.
> Ten sam język — w tym PRINT, GRAPH, INPUT, DELAY — ma działać na Linuksie i Windowsie.
> Agent nie może zakładać, że Nyota działa jak C, Python, Pascal, BASIC, AMOS,
> JavaScript, Rust ani Clojure. Źródłem prawdy jest ta specyfikacja oraz interpreter Nyoty.

## Status synchronizacji z kodem

**Audyt synchronizacji:** 2026-09-19, gałąź `nyota-v05-complete`.

Ten plik opisuje **zachowanie faktycznie zaimplementowane**, a nie plan języka. Przy sporze obowiązuje kolejność:

1. `src/nyota.c` — składnia, typy, parser, semantyka i wykonanie instrukcji;
2. `src/nyota_host.h` oraz implementacja hosta — kontrakt platformowy;
3. `tests/*.nyo` — regresja zachowania;
4. `docs/nyota.md` — opis dla człowieka.

Jeżeli dokument rozmija się z kodem lub regresją, jest to **błąd dokumentacji**. Elementy planowane, ale jeszcze niewdrożone, należą do [`docs/nyota_v05.md`](nyota_v05.md), a pełny kontrakt GUI do [`docs/nyotaui.md`](nyotaui.md).

Bieżące typy wartości interpretera: `INTEGER`, `FLOAT`, `STRING`, `BOOLEAN`, `LIST`, `DATE`, `MARK`, `TUPLE`, `TIME`, `RECORD`, `COLOR`.

---

## 0. Język, edytor, host

Trzy osobne rzeczy:

```text
Nyota     — język i interpreter
Tunga     — edytor (osobny projekt; potrafi uruchomić Nyotę)
AyoOS     — pierwszy system-host
```

Nyota nie jest modułem Tungi. VS Code jest drugim obsługiwanym edytorem, analogicznie do Tungi.

`PRINT`, `GRAPH`, `INPUT` i `DELAY` **należą do języka** i muszą działać
na każdym hoście, w tym na Linuksie. Host nie wycina tych poleceń.
Host tylko podłącza backend (AyoAPI, SDL, okno).

Nie ma Nyoty bez grafiki. Jest jeden język.

### 0.1. Nyota Platform Invariance Rule

To jest twarda zasada projektu.

> Poprawny kod źródłowy Nyoty nie może wymagać zmian zależnie od systemu
> operacyjnego. Ten sam program ma być używany bez zmian na AyoOS, Linuxie
> i Windowsie. Różnice platformowe są obowiązkiem hosta, ABI i toolchainu,
> a nie programisty Nyoty.

Ta sama zasada obejmuje planowane moduły natywne:
- `NC` — moduł C,
- `NZ` — moduł Zig,
- `NA` — moduł natywnego assemblera procesora.

`NC` i `NZ` mają mieć identyczne źródła na wszystkich oficjalnych hostach.
`NA` jest zależne od architektury CPU, ale nie od systemu operacyjnego:
dla tej samej architektury, np. x86-64, ten sam moduł NA ma działać na AyoOS,
Linuxie i Windowsie bez zmian w źródle.

Kod NC/NZ/NA nie powinien bezpośrednio używać AyoAPI, POSIX ani WinAPI.
Dostęp do systemu ma prowadzić przez wspólny kontrakt `NyotaNativeAPI`
oraz stabilne ABI Nyoty. To zachowuje przenośność źródła.

`NYASM` pozostaje czymś innym: jest przenośną maszyną wirtualną Nyoty,
nie natywnym assemblerem CPU.


Stan interpretera na 2026-09-17:

- <span style="color: #006A4E;">wcięcia: 4 spacje na poziom, tabulacja = błąd, wcięcie niebędące wielokrotnością 4 = błąd wykonane 2026-09-17</span>
- <span style="color: #006A4E;">nieznana instrukcja zgłasza błąd interpretera zamiast cichego ignorowania wykonane 2026-09-17</span>
- <span style="color: #006A4E;">BEGIN i END są obowiązkowe; brak pary albo kod poza blokiem to błąd, a nie ciche wykonanie całego pliku wykonane 2026-09-17</span>
- <span style="color: #006A4E;">`+` wymaga tego samego typu; `4 + "8"` i `"8" + 4` są błędem, nie zgadywaniem wykonane 2026-09-17</span>
- <span style="color: #006A4E;">`=` i `<>` porównują typ i wartość; `5 = "5"` oraz `5 = 5.0` są błędem typu wykonane 2026-09-17</span>
- <span style="color: #006A4E;">FLOAT: stała precyzja 3 miejsc, jawne ucinanie dalszych cyfr, poprawne liczby ujemne i kontrola przepełnień wykonane 2026-09-18</span>
- <span style="color: #006A4E;">typ DATE: literał `&lt;RRRR.MM.DD&gt;`, walidacja gregoriańska, `+`/`-` dni, `YEAR`/`MONTH`/`DAY`/`TODAY` wykonane 2026-09-17</span>
- <span style="color: #006A4E;">FUNCTION / PROCEDURE: parametry, VAR, RETURN, zakresy i wywołanie w wyrażeniu wykonane 2026-09-17</span>
- <span style="color: #006A4E;">IF / ELIF / ELSE jako jeden łańcuch: tylko pierwsza prawdziwa gałąź, ELSE nie odpala się sam wykonane 2026-09-17</span>
- <span style="color: #006A4E;">precedencja: `*` przed `+`, lewostronne `-`, nawiasy, NOT/AND/OR w wyrażeniu wykonane 2026-09-17</span>
- <span style="color: #006A4E;">WHILE (warunek BOOLEAN), CONTINUE, FOR STEP oraz operator MOD wykonane 2026-09-17</span>
- <span style="color: #006A4E;">MARK: literał i mutacje, REKEY, KEY/VALUE/VALUES/MLIST/MINFO, IN/FOR IN, algebra + - ><, EXTEND/CLEAR/REVERSE/SORT, kolumny MEXTEND/MINSERT/MDROP oraz statystyki wykonane 2026-09-17</span>
- <span style="color: #006A4E;">LIST: literały (także zagnieżdżone), indeksowanie INTEGER, `+` `-` `><`, IN/LEN, APPEND/EXTEND/REMOVE/CLEAR/REVERSE/SORT, FOR...IN, RANDINT/RANDFLT wykonane 2026-09-17</span>
- <span style="color: #006A4E;">TUPLE: literał, indeksowanie, LEN, IN, FOR...IN i konwersje LIST/TUPLE wykonane 2026-09-17</span>
- <span style="color: #006A4E;">TIME: TIME(), TIME(HH.MM.SS), HOUR/MINUTE/SECOND, H/M/S, TIME-TIME, porównania i SORT wykonane 2026-09-17</span>
- <span style="color: #006A4E;">TABLE: nazwana kontrolka prezentacji danych; kolumny i szerokości, czcionka/rozmiar, kolor tekstu, opcjonalne źródło LIST/TUPLE/MARK oraz TABLE_DATA wykonane 2026-09-17</span>
- <span style="color: #006A4E;">BUTTON: nazwana kontrolka GUI z geometrią, tekstem, czcionką/rozmiarem, kolorami tekstu/tła i BUTTON_CLICKED() wykonane 2026-09-17</span>
- <span style="color: #006A4E;">SPRITE: nazwany obiekt graficzny, pozycja/ruch/widoczność, animacja klatkowa, jawne rysowanie i kolizja prostokątna wykonane 2026-09-18</span>
- <span style="color: #006A4E;">FILE oraz DIR/LS przez kontrakt NyotaHost; pełny backend POSIX wykonane 2026-09-18</span>
- <span style="color: #006A4E;">SORT: jawny wybór AUTO/BUBBLE/INSERT/SELECT/MERGE/QUICK/HEAP/SHELL/COUNTING wykonane 2026-09-18</span>
- <span style="color: #006A4E;">NYASM: bezpieczna VM R0-R3 z INPUT/OUTPUT i MOV/ADD/SUB/MUL/DIV/MOD/STORE wykonane 2026-09-18</span>
- <span style="color: #006A4E;">NyotaUI: `WIN` oraz kontrolki `BUTTON`, `LABEL`, `PANEL`, `DAREA`, `CBOX`, `RADIO`, `COMBO`, `SEP`, `TABS`, `TAB`, `TAREA`, `SBAR`, `PBAR`, `EQBOX`, `SLIDER`, `STATBAR`, `TOOLBAR`, `TBOX`, `SPINBOX`, `LISTVIEW`, `TREEVIEW`, `SPLITTER`, `SCALE`, `CLOCK`, `ICONBUTTON`, `SWITCH`, `FRAME`, `INPUT`; wspólne `.CONFIG`, layout FREE/ROW/COL, AUTO/CENTER, PNG, gradienty, focus, UTF-8 i HiDPI — rdzeń i backend POSIX działają; AyoOS czeka na podpięcie backendu</span>
- <span style="color: #006A4E;">PRINT z wieloma argumentami (spacja między nimi, tylko do wyświetlenia) wykonane 2026-09-17</span>
- <span style="color: #006A4E;">`=N=` ucina do N miejsc, ten sam typ INTEGER/FLOAT wykonane 2026-09-17</span>
- Tunga pozostaje osobnym edytorem wywołującym interpreter Nyoty; integracja Tungi nie jest częścią tego repozytorium.
- <span style="color: #006A4E;">Linux: host POSIX z SDL2, SDL2_image, SDL2_ttf i fontconfig; terminal, GRAPH/SCREEN, FILE/DIR, SPRITE oraz pełna bieżąca warstwa NyotaUI są podłączone</span>
- Pomoc Tunga/AyoEdit znajduje się poza dostępnym repozytorium Nyoty; źródłem prawdy pozostaje reguła 4 spacji.

Pełny kontrakt NyotaUI: `docs/nyotaui.md`.

Testy regresyjne: `tests/` — w tym `date_arith.nyo`, `date_add.nyo`, `date_cmp.nyo`, `date_parts.nyo`, `date_leap_ok.nyo`, `date_leap_bad.nyo`, `date_gregorian.nyo`, `date_plus_date.nyo`.

---

## 1. Rola agenta

Jesteś agentem programowania dla AyoEdit i AyoOS.

Twoje zadania:

1. pomagać pisać kod w języku Nyota,
2. wyjaśniać błędy kompilatora/interpretera,
3. proponować poprawki zgodne ze składnią Nyota,
4. nie wymyślać nieistniejących konstrukcji języka,
5. pokazywać poprawki jako czytelny patch albo jako pełny poprawiony fragment,
6. nie usuwać logiki programu bez wyraźnego powodu,
7. zawsze szanować styl AyoOS: prostota, czytelność, kontrola typów, brak ukrytej magii.

---

## 2. Zasada nadrzędna: nie zgaduj Nyoty

Nyota jest językiem autorskim. Agent ma obowiązek traktować dokumentację Nyota jako źródło prawdy.

### Twarde reguły dla agenta

* Nie używaj składni C, Pythona, JavaScriptu, Pascala ani BASIC-a, jeżeli nie jest opisana jako poprawna składnia Nyota.
* Nie używaj średników.
* Nie używaj klamer `{}` jako bloków kodu.
* Nie używaj `==`.
* Nie używaj `!=`.
* Nie używaj małych liter dla słów kluczowych.
* Nie używaj tabulatorów.
* Nie zakładaj, że funkcja biblioteczna istnieje, jeżeli nie została opisana w dokumentacji.
* Jeżeli reguła nie jest opisana, napisz, że dokumentacja nie definiuje tej konstrukcji.

---

## 3. Rozszerzenie plików

Pliki źródłowe Nyota mają rozszerzenie:

```text
.nyo
```

---

## 4. Struktura programu

Kod wykonywalny programu musi być objęty blokiem:

```nyota
BEGIN
...
END
```

Interpreter egzekwuje tę ramę. Brak `BEGIN`, brak `END`, `END` przed `BEGIN`, więcej niż jedna para oraz instrukcja przed `BEGIN` albo po `END` kończą się błędem. Interpreter nie uruchamia całego pliku „na wszelki wypadek”.

Przed `BEGIN` można umieszczać wyłącznie deklaracje globalne, na przykład:

* `CONST`,
* `RECORD`,
* `FUNCTION`,
* `PROCEDURE`,
* `IMPORT`.

Przykład:

```nyota
CONST MAX_OKIEN := 10

PROCEDURE Powitanie():
    PRINT "Witaj w AyoOS"

BEGIN
PRINT "Start programu"
Powitanie()
END
```

---

## 5. Instrukcje i końce linii

Nyota nie używa średników.

Każda nowa linia oznacza nową instrukcję.

Poprawnie:

```nyota
VAR x := 5
PRINT x
```

Błędnie:

```nyota
VAR x := 5;
PRINT x;
```

---

## 6. Komentarze

Komentarz zaczyna się od znaku `#`.

```nyota
# To jest komentarz
VAR x := 5 # komentarz po instrukcji
```

---

## 7. Wielkość liter

Nyota rozróżnia wielkość liter.

Wszystkie słowa kluczowe muszą być pisane wielkimi literami.

Poprawnie:

```nyota
VAR x := 5
IF x = 5:
    PRINT "OK"
```

Błędnie:

```nyota
var x := 5
if x = 5:
    print "OK"
```

---

## 8. Bloki kodu i wcięcia

Nyota używa bloków opartych o dwukropek i wcięcia.

Zasady:

* linia kończąca się `:` otwiera blok,
* wnętrze nowego bloku musi zaczynać się dokładnie 4 spacje głębiej niż instrukcja otwierająca,
* skok o 8 lub więcej spacji bez pośredniego bloku jest błędem,
* wcięcie linii z kodem musi być wielokrotnością 4,
* tabulacja jest surowo zabroniona i powoduje błąd interpretera,
* powrót do wcześniejszego wcięcia zamyka blok.

Poprawnie:

```nyota
IF x > 0:
    PRINT "Dodatnia"
```

Błędnie — tabulator:

```nyota
IF x > 0:
	PRINT "Dodatnia"
```

Błędnie — zła liczba spacji (tu 2, nie wielokrotność 4):

```nyota
IF x > 0:
  PRINT "Dodatnia"
```

---

## 9. Zmienne

Zmienne deklaruje się słowem `VAR`.

Przypisanie wykonuje się operatorem `:=`.

```nyota
VAR a := 5
VAR b := 5.3
VAR tekst := "AyoOS"
VAR aktywne := TRUE
VAR lista := [4, 7, "f"]
```

---

## 10. Typy i blokowanie typu

Nyota wnioskuje typ na podstawie literału.

Raz ustalony typ zmiennej jest zablokowany i nie może zostać zmieniony.

Przykład błędny:

```nyota
VAR a := 5
a := "slowo"
```

Powód błędu: `a` została zablokowana jako `INTEGER`, a później próbowano przypisać `STRING`.

### 10.1. Typ DATE

`DATE` jest pełnoprawnym typem. Literał ma kanoniczny kształt `&lt;RRRR.MM.DD&gt;`
i jest walidowany od razu. Interpreter nie poprawia nieprawidłowej daty.

```nyota
VAR start := <2026.09.10>
VAR jutro := start + 1
VAR dni := <2026.09.17> - start
PRINT YEAR(start)
```

Reguły:

* rok przestępny według pełnej reguły gregoriańskiej (`2000` tak, `2100` nie),
* `DATE - DATE` daje `INTEGER` (dni),
* `DATE + INTEGER` i `DATE - INTEGER` przesuwają datę,
* `DATE + DATE` jest błędem,
* `=` / `<>` / `<` / `>` porównują daty chronologicznie i wymagają typu `DATE`,
* `TODAY()`, `YEAR()`, `MONTH()`, `DAY()` — `TODAY()` czyta czas systemowy.

<span style="color: #006A4E;">literał, walidacja, arytmetyka dni i funkcje DATE wykonane 2026-09-17</span>  
`DATE` uczestniczy także w `SORT`; `MARK` i `TIME` są już częścią bieżącej implementacji.

### 10.2. Typ TUPLE

`TUPLE` jest niemutowalną sekwencją indeksowaną od zera.

```nyota
VAR t := (10, "Ayo", TRUE)
PRINT LEN(t), t[1]
```

Obsługiwane są `LEN`, `IN`, `FOR ... IN`, indeksowanie oraz jawne konwersje `TUPLE(list)` i `LIST(tuple)`. Próba `t[0] := ...` jest błędem.

### 10.3. Typ TIME

`TIME()` zwraca lokalny czas hosta. Jawna wartość ma postać `TIME(HH.MM.SS)`.

```nyota
VAR t := TIME(14.20.05)
PRINT HOUR(t), MINUTE(t), SECOND(t)
VAR pozniej := t + H2 + M10 + S40
```

`TIME - TIME` zwraca różnicę w sekundach jako `INTEGER`. Przesunięcia używają jednostek `H`, `M`, `S`; wynik zawija się w obrębie doby. `TIME` można porównywać i sortować.

### 10.4. Typ COLOR

`COLOR` jest pełnoprawną wartością języka używaną przede wszystkim przez NyotaUI.

```nyota
VAR a := SAPPHIRE
VAR b := 0x0F52BA
VAR c := 0x0F52BA80
```

Nazwane kolory i wartości RGB/RGBA są kontraktem języka, a nie motywem hosta. `TRANSPARENT` i `BACKDROP` są specjalnymi trybami koloru/tła; backend może jawnie odrzucić `BACKDROP`, jeśli nie potrafi pobrać skomponowanej sceny pod obiektem. Pełna paleta znajduje się w `src/nyota_color.h`.

---

## 11. Typ BOOLEAN

Wartości logiczne zapisuje się wielkimi literami:

```nyota
TRUE
FALSE
```

Poprawnie:

```nyota
VAR gotowe := TRUE
```

Błędnie:

```nyota
VAR gotowe := true
```

---

## 12. Rzutowanie typów

Do konwersji typów używa się funkcji konwertujących. Interpreter nie zgaduje
konwersji przy `+` ani przy `=`.

```text
INT(x)     -> INTEGER
FLT(x)     -> FLOAT
STR(x)     -> STRING
STRING(x)  -> STRING   (nadal akceptowane)
BOOL(x)    -> BOOLEAN
```

Przykład:

```nyota
VAR a := "5"
VAR b := 6
PRINT INT(a) + b
```

`BOOL(0)` daje `FALSE`, każda inna liczba daje `TRUE`. `BOOLEAN` nie bierze
udziału w arytmetyce bez `INT()`.

Funkcje zwracające wartość zawsze wywołuje się z nawiasami.

---

## 13. Instrukcje kontra funkcje

W Nyocie obowiązuje twarda zasada wywołań:

### Instrukcje systemowe i akcje

Instrukcje nie zwracają wartości i wywołuje się je bez nawiasów.

Przykłady:

```nyota
PRINT "Witaj"
PRINT "A", 5, "B"
DELAY 500
GOTOXY 10, 5
GRAPH 6
WIN glowne, ROOT, [400, 300], [100, 100], TRUE
```

### Funkcje zwracające wartość

Funkcje zwracają wartość i zawsze wymagają nawiasów.

Przykłady:

```nyota
VAR liczba := INT("123")
VAR dlugosc := LEN("AyoOS")
VAR tekst := INPUT("Podaj imie: ")
```

---

## 14. Operatory porównania

Nyota używa `=` do porównania.

Operator `==` nie istnieje.

`=N=` porównuje liczby tego samego typu po ucięciu do N miejsc:

```nyota
IF 2.5678 =2= 2.5611:
    PRINT "Rowne"
```

```nyota
IF x = 5:
    PRINT "x jest rowne 5"
```

Nierówność zapisuje się jako:

```nyota
<>
```

albo:

```nyota
><
```

Przykład:

```nyota
IF x <> 5:
    PRINT "x nie jest piatka"
```

Operatory mniejsze/równe i większe/równe mogą mieć dwa warianty:

```nyota
<=
=<
>=
=>
```

Przykład:

```nyota
IF x =< 10:
    PRINT "x jest mniejsze lub rowne 10"
```

---

## 15. Porównanie typów

Operator `=` porównuje zarówno wartość, jak i typ.

Porównanie różnych typów powoduje błąd interpretera.

Przykład potencjalnie błędny:

```nyota
VAR a := 5
VAR b := "5"
IF a = b:
    PRINT "To samo"
```

Powód: `a` to `INTEGER`, a `b` to `STRING`.

To samo dotyczy `5 = 5.0`. Poprawnie: `FLT(5) = 5.0` albo `INT(5.0) = 5`.

`<>` i `><` też wymagają tego samego typu.

---

## 16. Operatory logiczne

Nyota używa słów:

```nyota
AND
OR
NOT
```

Operator `NOT` ma niższy priorytet niż porównania i działa na cały najbliższy warunek.

```nyota
IF NOT y = 10:
    PRINT "y nie jest rowne 10"
```

Powyższy zapis oznacza:

```text
NOT (y = 10)
```

---

## 17. Precedencja operatorów

Kolejność od najwyższego do najniższego priorytetu:

1. `!`
2. funkcje `()`
3. `^`, `^^`
4. `*`, `/`, `MOD`, `/%`, `%`
5. `+`, `-`
6. `SHL`, `SHR`
7. `BAND`
8. `BXOR`
9. `BOR`
10. porównania: `=`, `<>`, `><`, `<`, `>`, `<=`, `>=`, `=<`, `=>`
11. `NOT`
12. `AND`
13. `OR`

Parser wyrażeń egzekwuje cały powyższy porządek. `2 * 3 + 4` daje `10`,
`2 + 3 * 4` daje `14`, `10 - 3 - 2` daje `5`. `!`, `SHL`, `SHR`, `BAND`,
`BXOR` i `BOR` są wykonywane przez ten sam parser precedencji.

---

## 18. Operatory matematyczne

```text
+    dodawanie
-    odejmowanie
*    mnożenie
/    dzielenie
^    potęgowanie
^^   pierwiastkowanie
%    procent
MOD  modulo / reszta z dzielenia
/%   modulo (nadal akceptowane)
!    silnia postfiksowa
SHL  przesunięcie bitowe w lewo
SHR  przesunięcie bitowe w prawo
BAND bitowe AND
BOR  bitowe OR
BXOR bitowe XOR
```

Ważne:

* `%` zawsze oznacza procent,
* modulo zapisuje się `MOD`; `/%` jest nadal akceptowane,
* `!` jest operatorem postfiksowym, działa na nieujemnym `INTEGER` i zgłasza przepełnienie,
* `SHL` / `SHR` wymagają `INTEGER` i przesunięcia 0..31,
* `BAND`, `BXOR`, `BOR` działają na 32-bitowych wartościach `INTEGER`.

Przykłady:

```nyota
PRINT 2 ^ 3       # 8
PRINT 9 ^^ 2      # pierwiastek drugiego stopnia z 9
PRINT 200 % 50    # 50% z 200
PRINT 10 MOD 3    # 1
PRINT 10 /% 3     # 1  (alias)
PRINT 5!          # 120
```

---

## 19. Dzielenie

Jeżeli oba argumenty są typu całkowitego, dzielenie `/` daje wynik całkowity.

```nyota
PRINT 9 / 2       # 4
PRINT 9.0 / 2     # 4.5
```

---

## 20. Funkcje matematyczne

Funkcje matematyczne wymagają nawiasów.

```nyota
FLOOR(x)
CEIL(x)
ROUND(x)
SIN(x)
COS(x)
TG(x)
CTG(x)
ASIN(x)
ACOS(x)
ATG(x)
ACTG(x)
```

---

## 21. IF / ELIF / ELSE

Instrukcje warunkowe:

```nyota
IF punkty > 100:
    PRINT "Zloty medal!"
ELIF punkty > 50:
    PRINT "Srebrny medal!"
ELSE:
    PRINT "Sprobuj jeszcze raz."
```

Każdy blok wymaga dwukropka i wcięcia 4 spacji.

Łańcuch `IF` / `ELIF` / `ELSE` jest jedną konstrukcją. Po wykonaniu pierwszej
prawdziwej gałęzi pozostałe `ELIF` i `ELSE` są pomijane. `ELIF` albo `ELSE`
bez poprzedzającego `IF` na tym samym wcięciu jest błędem.

---

## 22. Pętle

Nyota obsługuje:

* `FOR` z opcjonalnym `STEP`,
* `WHILE`,
* `REPEAT ... UNTIL`,
* `DO`,
* `EXIT`,
* `CONTINUE`.

### FOR

```nyota
FOR i := 1 TO 10:
    PRINT i

FOR i := 0 TO 100 STEP 5:
    PRINT i

FOR i := 10 TO 1 STEP -1:
    PRINT i
```

Nie ma `NEXT`. Pętla kończy się powrotem wcięcia. Brak `STEP` oznacza krok `1`
albo `-1`, gdy start jest większy od końca.

### WHILE

Warunek jest sprawdzany **przed** każdą iteracją i musi być `BOOLEAN`.

```nyota
VAR x := 0
WHILE x < 10:
    PRINT x
    x := x + 1
```

`WHILE 1:` jest błędem typu. Jawna konwersja: `WHILE BOOL(x):`.

### REPEAT / UNTIL

```nyota
REPEAT:
    PRINT "Wykonuje sie przynajmniej raz"
UNTIL x = 10
```

### DO

```nyota
DO:
    PRINT "Petla nieskonczona"
```

### EXIT

`EXIT` przerywa tylko najbliższą aktywną pętlę.

```nyota
FOR i := 1 TO 10:
    DO:
        EXIT
```

W tym przykładzie `EXIT` przerywa tylko wewnętrzną pętlę `DO`, a nie cały `FOR`.

`CONTINUE` pomija resztę bieżącej iteracji najbliższej pętli. Poza pętlą
`CONTINUE` i `EXIT` są błędami.

Do opuszczania funkcji lub procedury służy `RETURN`.

---

## 23. SWITCH / CASE

```nyota
SWITCH klawisz:
    CASE "A":
        PRINT "Wcisnieto A"
    CASE "B":
        PRINT "Wcisnieto B"
    CASE _:
        PRINT "Nierozpoznany klawisz"
```

`CASE _:` oznacza przypadek domyślny.

Symbol `_` jest zarezerwowany i nie może być nazwą zmiennej.

---

## 24. Procedury

Procedura nie zwraca wartości.

Definicja:

```nyota
PROCEDURE Nazwa():
    PRINT "Dzialam"
```

W procedurze `RETURN` jest zabroniony. Interpreter wykonuje też pre-scan kontraktu funkcji: wykrywa brak `RETURN` oraz różne statycznie rozpoznawalne typy na różnych ścieżkach. Typ zwracany jest dodatkowo blokowany podczas wykonania.

Błędnie:

```nyota
PROCEDURE Test():
    RETURN 5
```

---

## 25. Funkcje

Funkcja zwraca wartość.

<span style="color: #006A4E;">wywołanie FUNCTION w wyrażeniu, parametry, RETURN i zakresy wykonane 2026-09-17</span>

Definicja:

```nyota
FUNCTION Dodaj(a, b):
    RETURN a + b
```

Zasady:

* `FUNCTION` musi zakończyć się `RETURN`,
* `RETURN` musi zawierać wartość,
* funkcja musi zawsze zwracać ten sam typ,
* różne typy zwracane z jednej funkcji powodują błąd.

Błędnie:

```nyota
FUNCTION Test(x):
    IF x > 0:
        RETURN 5
    ELSE:
        RETURN "blad"
```

Powód: funkcja raz zwraca `INTEGER`, a raz `STRING`.

---

## 26. Parametry przez referencję

Do przekazywania argumentów przez referencję używa się `VAR` w liście parametrów.

```nyota
PROCEDURE Zwieksz(VAR liczba):
    liczba := liczba + 1
```

Parametr przekazany przez `VAR` pozwala procedurze modyfikować oryginalną zmienną.

---

## 27. Zakres zmiennych

Zmienne utworzone wewnątrz `PROCEDURE` lub `FUNCTION` są lokalne.

Przesłanianie zmiennej globalnej przez lokalną jest dozwolone.

---

## 28. CONST

Stałe definiuje się przez `CONST`.

```nyota
CONST MAX_OKIEN := 10
```

Stałe można umieszczać przed `BEGIN` albo na początku bloku `BEGIN`.

---

## 29. RECORD

Rekord definiuje własną strukturę danych.

```nyota
RECORD Postac:
    x := 0
    y := 0
    hp := 100
    imie := "Nieznany"
```

Instancję tworzy się jak funkcję:

```nyota
VAR boss := Postac()
boss.imie := "Wielki Smok"
```

Pola mają twardo zablokowane typy na podstawie wartości domyślnych. Definicje `RECORD` są skanowane przed uruchomieniem programu, konstruktor nie przyjmuje argumentów, a przypisanie pola innego typu jest błędem.

---

## 30. WITH

`WITH` skraca zapis dostępu do pól obiektu.

```nyota
WITH boss:
    hp := 5000
    imie := "Smok"
```

`WITH` działa na oryginalnym obiekcie, nie na kopii.

Wewnątrz `WITH` nazwy odnoszą się najpierw do pól obiektu, potem do zakresu zewnętrznego. Przypisanie do pola zmienia oryginalny rekord.

---

## 31. LIST

Lista może przechowywać wiele elementów.

Indeksy zaczynają się od zera.

```nyota
VAR a := [1, 2, "trzy"]
PRINT a[0]
a[1] := 5
```

Operacje:

```nyota
APPEND a, "cztery"
REMOVE a[0]
REMOVE a, "trzy"
REMOVE a, 5, ALL
EXTEND a, b
REVERSE a
SORT a
SORT a, DESC
CLEAR a
PRINT LEN(a)
```

Operatory zwracają nową listę; instrukcje mutują istniejącą:

```nyota
VAR c := a + b
VAR d := a - b
VAR e := a >< b
```

Iteracja działa bezpośrednio po elementach listy:

```nyota
FOR element IN a:
    PRINT element
```

Losowe listy liczbowe:

```nyota
VAR liczby := RANDINT(10, 1, 100)
VAR pomiary := RANDFLT(10, 0, 1, 3)
```

`RANDFLT` ma domyślną precyzję 2; jawna precyzja może wynosić `0..3`.

Porównanie elementów jest ścisłe typowo: `5`, `"5"` i `5.0` są różne.

Zasady:

* `APPEND`, `REMOVE`, `EXTEND`, `REVERSE`, `SORT`, `CLEAR` są instrukcjami mutującymi,
* `CONST` zawierający LIST nie może być mutowany tymi instrukcjami ani przez indeks,
* `LEN(a)` jest funkcją, a `x IN a` używa ścisłej tożsamości typu i wartości,
* indeks LIST musi być typu `INTEGER`; nie ma automatycznej konwersji indeksu,
* `REMOVE a[i]` usuwa po indeksie; `REMOVE a, wartosc` po wartości; `ALL` usuwa wszystkie wystąpienia,
* `SORT` obsługuje jednorodne listy `INTEGER`, `FLOAT`, `STRING` i `DATE`; `ASC` jest domyślne, `DESC` odwraca kierunek,
* `+`, `-` i `><` zwracają nową listę i nie mutują operandów,
* bieżący interpreter ma limit implementacyjny 64 elementów jednej LIST; nie jest to deklarowany limit języka.

---

## 32. MARK

`MARK` to autorska struktura Nyota łącząca słownik i tabelę danych.

<span style="color: #006A4E;">pełny podstawowy MARK: odczyt/mutacje, algebra, iteracja, sortowanie, operacje kolumnowe i statystyki wykonane 2026-09-17</span>

Format:

```nyota
VAR kraje := {3| "Polska" | "Warszawa" | "Wisla" , "Niemcy" | "Berlin" | "Ren" }
```

Pierwsza liczba określa całkowitą liczbę kolumn: klucz + wartości.

Odczyt:

```nyota
PRINT kraje["Polska"][0]
PRINT kraje["Niemcy"][1]
```

Dodanie lub nadpisanie wiersza:

```nyota
kraje["Francja"] := ["Paryz", "Sekwana"]
```

Usunięcie wiersza:

```nyota
DELETE kraje["Niemcy"]
```

Zasady:

* klucze są porównywane po typie i wartości,
* `"5"`, `5` i `TRUE` to różne klucze,
* wartości w MARK są przechowywane jako LIST,
* przypisywana LIST musi mieć zgodną liczbę kolumn,
* `DELETE` działa wyłącznie na MARK,
* nie mylić `DELETE` z `REMOVE`,
* `REKEY m, old, new` zmienia klucz bez zmiany pozycji wiersza,
* `FOR key IN m:` iteruje po kluczach w bieżącej kolejności,
* `+`, `-` i `><` zwracają nowy MARK; `EXTEND`, `REVERSE`, `SORT`, `CLEAR`, `MEXTEND`, `MINSERT`, `MDROP` mutują,
* `VALUES(m, row, ...)` zwraca LIST wybranych kolumn; zakres `2:7` jest domknięty,
* `MLIST(m, KEY)` zwraca klucze, a `MLIST(m, n)` wskazaną kolumnę wartości,
* `SORT m, KEY[, DESC]` oraz `SORT m, VALUE n[, DESC]` sortują wiersze,
* `SUM`, `AVG`, `MED`, `MIN`, `MAX`, `MODE`, `MODECOUNT`, `COUNT` działają kolumnowo,
* przy remisie `MODE` wybiera pierwszą wartość w aktualnej kolejności MARK-a; `MODECOUNT` zwraca jej liczność,
* indeksy wierszy/kolumn dla funkcji MARK wymagają `INTEGER`, bez konwersji niejawnej,
* `CONST` zawierający MARK nie może być mutowany.

---

## 33. LEN()

`LEN()` działa wyłącznie na:

* `STRING`,
* `LIST`,
* `MARK`,
* `TUPLE`.

Użycie `LEN()` na innych typach, na przykład `RECORD`, powoduje błąd.

Dla `MARK`, `LEN()` zwraca liczbę kluczy/wierszy.

---

## 34. STRING

Łączenie tekstów:

```nyota
VAR powitanie := "Witaj " + imie + "!"
```

Slicing:

```nyota
PRINT imie[0:3]
```

Zmiana wielkości liter:

```nyota
PRINT UPPER("ayoos")
PRINT LOWER("AYOOS")
```

Szukanie fragmentu:

```nyota
PRINT FIND("AyoOS", "OS")
```

`FIND(tekst, szukany_fragment)` zwraca indeks od zera albo `-1`.

---

## 35. Nowa linia w tekście

Nyota nie używa `\n` jako znacznika nowej linii w stringach.

Do nowej linii służy:

```text
~/
```

Liczba slashy określa liczbę złamań linii:

```text
~/      jedna nowa linia
~//     dwie nowe linie
~///    trzy nowe linie
```

Przykład:

```nyota
PRINT "Tytul programu~//Autor: Marek~/Wersja: 1.0"
```

---

## 36. INPUT

`INPUT()` jest funkcją, bo zwraca tekst.

```nyota
VAR imie := INPUT("Jak masz na imie? ")
PRINT "Witaj " + imie
```

Zasady:

* `INPUT()` zawsze zwraca `STRING`,
* może zwrócić pusty string `""`,
* wymaga nawiasów.

---

## 37. DELAY

`DELAY` jest instrukcją.

```nyota
DELAY 500
```

Czas podaje się w milisekundach.

---

## 38. Obsługa błędów

Nyota używa konstrukcji:

```nyota
ON ERROR CALL NazwaProcedury
```

Interpreter udostępnia zmienną systemową:

```nyota
ERR_CODE
```

Przykład:

```nyota
PROCEDURE ObsluzBlad():
    PRINT "Blad systemu: " + STRING(ERR_CODE)

BEGIN
ON ERROR CALL ObsluzBlad
END
```

`ERR_CODE` jest systemowym `INTEGER` tylko do odczytu. Dla błędów samego interpretera ma obecnie kod `1`; handler jest wywoływany synchronicznie i ma ochronę przed rekursją błędów.

---

## 39. EVERY

`EVERY` pozwala cyklicznie wywoływać procedury.

```nyota
EVERY 1000 CALL OdswiezZegar
```

Anulowanie:

```nyota
CANCEL EVERY OdswiezZegar
```

Zasady:

* `EVERY` jest schedulerem zdarzeń,
* nie uruchamia prawdziwych wątków,
* zdarzenia wykonują się pomiędzy iteracjami głównej pętli,
* zdarzenia wykonują się w kolejności deklaracji,
* `CANCEL EVERY` nie przerywa aktualnie wykonywanego wywołania,
* procedura schedulera nie może mieć parametrów,
* bieżący limit implementacji to 16 zdarzeń,
* scheduler nie tworzy lawiny zaległych callbacków po dłuższej operacji.

---

## 40. IMPORT

Importowanie pliku:

```nyota
IMPORT "sciezka_do_pliku.nyo"
```

Zasady:

* `IMPORT` umieszcza się na początku pliku, przed `BEGIN`,
* plik importowany nie może zawierać `BEGIN` ani `END`,
* plik importowany nie może zawierać instrukcji wykonywalnych,
* dozwolone są wyłącznie deklaracje: `CONST`, `RECORD`, `FUNCTION`, `PROCEDURE`,
* duplikacja nazw `FUNCTION`, `PROCEDURE` lub `RECORD` powoduje błąd,
* import zagnieżdżony jest obecnie zabroniony,
* import jest rozwijany przed walidacją i pre-scanem deklaracji.

---

## 40a. STATE i proste prymitywy NN

Bieżący interpreter zawiera także małe prymitywy stanu i prostej sieci liczbowej. Są zaimplementowane i dlatego należą do opisu bieżącego kodu.

```nyota
STATE_CREATE 0
STATE_SET 0, "gotowe"
PRINT STATE_GET(0)

NN_CREATE 0, 4
NN_TRAIN 0, [100, 200, 300, 400], 700
PRINT NN_PREDICT(0, [100, 200, 300, 400])
```

Limity implementacyjne: 32 sloty `STATE`, 4 obiekty `NN`, maksymalnie 16 wejść/wag na obiekt NN. `NN_PREDICT()` zwraca `INTEGER` ograniczony do zakresu 0..1000.

Dodatkowe funkcje narzędziowe obecne w interpreterze: `RANDOM(max)`, `CHOOSE(list)`, `WEIGHTED(items, weights)`, `TOKENS(text)`, `MATCH(text, pattern)`, `SIMILARITY(a, b)`, `FUZZY(value, min, max)`, `SIMPLIFY(text)`, `NEAREST(x, y, points)`, `VISIBLE(x0, y0, x1, y1, rects)` i `PATHFIND(sx, sy, tx, ty)`. Są to funkcje wykonawcze obecnego interpretera, nie wpisy roadmapy.

---

## 41. GRAPH

Tryb graficzny ustawia się instrukcją `GRAPH`.

```nyota
GRAPH 6
```

Tryby:

```text
GRAPH 1 : 640 x 480
GRAPH 2 : 800 x 600
GRAPH 3 : 1024 x 768
GRAPH 4 : 1280 x 1024
GRAPH 5 : 1600 x 900
GRAPH 6 : 1920 x 1080
```

Po wejściu w tryb `GRAPH`, użycie `PRINT` lub `GOTOXY` powoduje twardy błąd interpretera.

---

## 42. SCREEN

Tworzenie ekranu off-screen:

```nyota
SCREEN <id> OPEN <szerokosc>, <wysokosc>, <tryb_kolorow>
```

Ustawienie aktywnego celu rysowania:

```nyota
SCREEN <id> SET
```

Zasady:

* `SCREEN 0` to fizyczny ekran systemu,
* `SCREEN 0` jest aktywny domyślnie,
* `SCREEN 0` nie może zostać zamknięty ani otwarty ponownie,
* ekran off-screen musi mieć identyczną rozdzielczość jak zadeklarowany `GRAPH`,
* ID screenów muszą być unikalne i dodatnie,
* przekroczenie limitu screenów powoduje błąd,
* `SCREEN <id> SET` dla nieistniejącego ekranu powoduje błąd,
* host POSIX/SDL2 implementuje `SCREEN` jako render target; zmiana `GRAPH` unieważnia wcześniejsze cele off-screen.

---

## 43. NyotaUI — WIN i kontrolki

`WIND_OPEN` nie jest instrukcją bieżącego interpretera. Aktualny model okien to `WIN` i kontrolki NyotaUI.

```nyota
WIN glowne, ROOT, [900, 620], CENTER, TRUE

FRAME panel, glowne, [360, 260], [20, 20], TEXT="Ustawienia"
FRAME panel.CONFIG:
    LAYOUT = COL
    GAP = 10
    LPADX = 16
    LPADY = 28

INPUT szukaj, panel, [300, 38], AUTO, TYPE=SEARCH
SWITCH wifi, panel, [92, 34], AUTO, VALUE=TRUE
BUTTON zapisz, panel, [140, 38], AUTO, TEXT="Zapisz"
```

Bieżące rodzaje kontrolek: `BUTTON`, `LABEL`, `PANEL`, `DAREA`, `CBOX`, `RADIO`, `COMBO`, `SEP`, `TABS`, `TAB`, `TAREA`, `SBAR`, `PBAR`, `EQBOX`, `SLIDER`, `STATBAR`, `TOOLBAR`, `TBOX`, `SPINBOX`, `LISTVIEW`, `TREEVIEW`, `SPLITTER`, `SCALE`, `CLOCK`, `ICONBUTTON`, `SWITCH`, `FRAME`, `INPUT`.

Kontrolki używają rodzica, rozmiaru, pozycji `AUTO`/`CENTER` albo `[x,y]`, właściwości inline oraz opcjonalnego bloku `.CONFIG:`. Szczegółowa lista właściwości, walidacja i model hosta są normatywnie opisane w [`docs/nyotaui.md`](nyotaui.md).

### 43a. BUTTON

Podstawowa bieżąca forma `BUTTON` należy do NyotaUI:

```nyota
BUTTON zapisz, panel, [160, 48], AUTO, TEXT="Zapisz"
BUTTON zapisz.CONFIG:
    BG = SAPPHIRE
    CTEXT = WHITE
    RADIUS = 8
```

`BUTTON_CLICKED(zapisz)` zwraca `BOOLEAN`. Interpreter zachowuje także starszą składnię `BUTTON` związaną z `GRAPH` jako zgodność przejściową; nie jest ona podstawowym modelem nowych aplikacji NyotaUI.

Funkcje stanu NyotaUI obejmują m.in. rodziny `*_SET`, `*_VALUE`, `*_INDEX`, `*_TEXT`, `*_CHANGED` oraz funkcje specyficzne dla kontrolek, np. `EQBOX_SET`, `EQBOX_BAR`, `CLOCK_SET`, `CLOCK_NEEDLE`, `CLOCK_VALUE`, `CLOCK_VALUES`, `INPUT_VALID`, `DAREA_DROPPED` i `DAREA_ITEMS`. Dokładne sygnatury są w `docs/nyotaui.md`.

---
## 43b. SPRITE

`SPRITE` jest nazwanym obiektem graficznym. Nie jest typem zmiennej Nyoty.

Definicja zasobu i początkowej geometrii:

```nyota
SPRITE gracz, "gfx/gracz.bmp", 100, 120, 32, 32
```

Składnia:

```text
SPRITE nazwa, obraz, x, y, szerokosc, wysokosc
```

`SPRITE` wymaga wcześniejszego `GRAPH`. Nazwa jest logicznym identyfikatorem,
tak jak dla `BUTTON` i `TABLE`. Bieżący backend POSIX/SDL2 ładuje obrazy BMP
przez `SDL_LoadBMP`; kontrakt NyotaHost nie narzuca formatu innym hostom.

Zmiana stanu:

```nyota
SPRITE_POS gracz, 200, 150
SPRITE_MOVE gracz, 5, -2
SPRITE_HIDE gracz
SPRITE_SHOW gracz
SPRITE_DELETE gracz
```

Rysowanie jest jawne:

```nyota
SPRITE_DRAW gracz
```

To świadoma reguła bieżącego, natychmiastowego modelu grafiki Nyoty.
`SPRITE_MOVE` i `SPRITE_POS` nie czyszczą starej klatki i nie rysują automatycznie.
W pętli gry program zwykle wykonuje `CLEAR`, rysuje tło i potem `SPRITE_DRAW`.

Funkcje stanu:

```nyota
VAR x := SPRITE_X(gracz)
VAR y := SPRITE_Y(gracz)
VAR w := SPRITE_W(gracz)
VAR h := SPRITE_H(gracz)
VAR pokazany := SPRITE_VISIBLE(gracz)
```

### Animacja klatkowa

Animacja korzysta z poziomego sprite sheetu z klatkami o równej szerokości.
Liczbę klatek i czas jednej klatki w milisekundach ustala:

```nyota
SPRITE_ANIM gracz, 6, 100
```

Po `SPRITE_ANIM` animacja startuje od klatki `0` i domyślnie zapętla się.
Sterowanie:

```nyota
SPRITE_PLAY gracz
SPRITE_STOP gracz
SPRITE_FRAME gracz, 3
```

`SPRITE_FRAME` ustawia konkretną klatkę i zatrzymuje automatyczne odtwarzanie.
Indeksy klatek zaczynają się od `0`. Funkcje:

```nyota
VAR klatka := SPRITE_FRAME(gracz)
VAR gra := SPRITE_PLAYING(gracz)
```

`SPRITE_FRAME()` zwraca bieżący indeks jako `INTEGER`, a
`SPRITE_PLAYING()` zwraca `BOOLEAN`. Klatka jest aktualizowana na podstawie
zegara hosta; `SPRITE_DRAW` rysuje aktualną klatkę. Bieżąca implementacja
dopuszcza `1..256` klatek i czas klatki `1..60000 ms`.

Backend POSIX/SDL2 wycina klatki poziomo z jednego BMP. Kontrakt NyotaHost
udostępnia osobny callback rysowania klatki, więc host AyoOS może użyć własnego
formatu obrazu bez zmiany składni języka.

Kolizja prostokątów AABB:

```nyota
IF SPRITE_HIT(gracz, przeciwnik):
    # reakcja programu
```

`SPRITE_HIT()` używa zapisanej geometrii `x/y/w/h`. Widoczność nie zmienia
geometrii kolizji. Funkcje przyjmują nazwę SPRITE jako identyfikator albo STRING.

---

## 44. GOTOXY

Pozycjonowanie kursora tekstowego:

```nyota
GOTOXY 10, 5
```

Parametry:

```text
X = kolumna
Y = wiersz
```

Nie używać po wejściu w tryb `GRAPH`.

---

## 45. Rysowanie grafiki

Instrukcje graficzne nie wymagają nawiasów.

```nyota
CLEAR r, g, b
BOX x, y, szerokosc, wysokosc, r, g, b
CIRCLE x, y, promien, r, g, b
ELLIPSE x, y, promien_x, promien_y, r, g, b
LINE x1, y1, x2, y2, r, g, b
TRIANGLE x1, y1, x2, y2, x3, y3, r, g, b
QUAD x1, y1, x2, y2, x3, y3, x4, y4, r, g, b
PENTAGON x1, y1, x2, y2, x3, y3, x4, y4, x5, y5, r, g, b
EGG x, y, promien_x, promien_y, kat, r, g, b
```

Przykłady:

```nyota
CLEAR 0, 0, 0
BOX 10, 10, 100, 50, 255, 0, 0
CIRCLE 150, 150, 30, 0, 255, 0
LINE 10, 10, 200, 200, 255, 255, 255
EGG 300, 300, 40, 60, 45, 255, 200, 100
```

---

## 46. LOAD / BANK

Zasoby można ładować do banków RAM.

```nyota
LOAD "ikona.ayoprv" INTO BANK 1
```

Banki służą do przechowywania grafik, dźwięków i danych w RAM.

---

## 46a. FILE — operacje plikowe

FILE jest częścią języka, a wykonanie operacji należy do hosta przez \`NyotaHost\`.
Rdzeń interpretera nie otwiera plików bezpośrednio przez POSIX ani nie dotyka
struktur systemu plików AyoOS.

Instrukcje:

\`\`\`nyota
FILE_WRITE "dane.txt", "Ayo"
FILE_APPEND "dane.txt", "OS"
FILE_COPY "dane.txt", "kopia.txt"
FILE_MOVE "kopia.txt", "archiwum.txt"
FILE_DELETE "archiwum.txt"
\`\`\`

Funkcje:

\`\`\`nyota
VAR tekst := FILE_READ("dane.txt")
VAR jest := FILE_EXISTS("dane.txt")
VAR bajty := FILE_SIZE("dane.txt")
\`\`\`

Zasady bieżącej implementacji:
- ścieżki i dane tekstowe są \`STRING\`,
- \`FILE_EXISTS()\` zwraca \`BOOLEAN\`,
- \`FILE_SIZE()\` zwraca \`INTEGER\`,
- \`FILE_READ()\` czyta tekst do STRING; obecny limit implementacji to 511 bajtów,
- plik zawierający bajt NUL nie jest traktowany jako STRING,
- błąd hosta jest jawnym błędem Nyoty.

Host POSIX implementuje cały powyższy zestaw. Host AyoOS ma ten sam kontrakt,
ale callbacki FILE/DIR muszą zostać podłączone do AyoAPI/VFS po stronie hosta.

---

## 46b. DIR / LS — katalogi

Instrukcje:

\`\`\`nyota
DIR_CREATE "dane"
DIR_COPY "dane", "kopia"
DIR_MOVE "kopia", "archiwum"
DIR_DELETE "archiwum"
\`\`\`

Funkcje:

\`\`\`nyota
VAR jest := DIR_EXISTS("dane")
VAR nazwy := DIR_LIST("dane")
VAR nazwy2 := LS("dane")
\`\`\`

\`DIR_LIST()\` oraz \`LS()\` mają tę samą semantykę: zwracają \`LIST\` nazw
wpisów posortowaną rosnąco. Nie wypisują katalogu na ekran.

W referencyjnym hoście POSIX:
- \`DIR_DELETE\` usuwa tylko pusty katalog,
- \`DIR_COPY\` kopiuje rekurencyjnie katalogi i zwykłe pliki,
- kopiowanie nie podąża za symlinkami i odrzuca pliki specjalne,
- wynik listowania ma bieżący limit bufora hosta 8192 bajtów i limit LIST Nyoty.

---

## 46c. SORT — jawny wybór algorytmu

Podstawowe formy pozostają najprostsze:

\`\`\`nyota
SORT dane
SORT dane, DESC
\`\`\`

Działa również dydaktyczny, jawny wybór algorytmu:

\`\`\`nyota
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
\`\`\`

Kierunki: \`ASC\` (domyślny), \`DESC\`, \`REVERSE\` = malejąco.
\`AUTO\` pozwala interpreterowi wybrać algorytm.

Typy sortowalne w LIST: \`INTEGER\`, \`FLOAT\`, \`STRING\`,
\`DATE\`, \`TIME\`. Lista mieszanych typów jest błędem.

\`COUNTING\` przyjmuje tylko \`INTEGER\` lub \`BOOLEAN\`; bieżący limit
implementacji to zakres maksymalnie 4096 wartości. Pozostałe algorytmy nie
mają tego ograniczenia.

---

## 46d. NYASM — bezpieczny wirtualny assembler

NYASM nie jest natywnym assemblerem procesora. Jest kontrolowaną maszyną
wirtualną Nyoty z czterema rejestrami:

\`\`\`text
R0 R1 R2 R3
\`\`\`

Przykład:

\`\`\`nyota
BEGIN
VAR a := 7
VAR b := 5

NYASM INPUT a, b OUTPUT wynik:
    MOV R0, a
    MUL R0, b
    ADD R0, 3
    STORE wynik, R0

PRINT wynik
END
\`\`\`

\`ASM\` jest aliasem nagłówka \`NYASM\`.

Instrukcje VM:

\`\`\`text
MOV Rn, operand
ADD Rn, operand
SUB Rn, operand
MUL Rn, operand
DIV Rn, operand
MOD Rn, operand
STORE output, Rn
\`\`\`

Operand może być rejestrem, literałem INTEGER, \`TRUE\`/\`FALSE\` albo
zmienną wymienioną w \`INPUT\`.

Reguły bezpieczeństwa:
- rejestry startują od zera,
- odczyt zmiennej spoza \`INPUT\` jest błędem,
- zapis spoza \`OUTPUT\` jest błędem,
- każdy zadeklarowany OUTPUT musi otrzymać \`STORE\`,
- istniejący OUTPUT musi być INTEGER albo BOOLEAN i nie może być CONST,
- nieistniejący OUTPUT jest tworzony jako INTEGER,
- VM wykrywa przepełnienie INTEGER i dzielenie/MOD przez zero,
- NYASM nie alokuje pamięci Nyoty, nie wykonuje syscalli, nie ma skoków do
  linii Nyoty i nie wywołuje FUNCTION/PROCEDURE.

Natywny x86-64 ASM/XASM nie ma jeszcze zatwierdzonego kontraktu i nie jest
częścią tej implementacji.

---

## 47. Typowe błędy, które agent ma wykrywać

### Błąd 1: średnik

```nyota
VAR x := 5;
```

Poprawka:

```nyota
VAR x := 5
```

### Błąd 2: operator `==`

```nyota
IF x == 5:
    PRINT "OK"
```

Poprawka:

```nyota
IF x = 5:
    PRINT "OK"
```

### Błąd 3: operator `!=`

```nyota
IF x != 5:
    PRINT "OK"
```

Poprawka:

```nyota
IF x <> 5:
    PRINT "OK"
```

### Błąd 4: zła liczba spacji

```nyota
IF x > 0:
  PRINT "OK"
```

Poprawka:

```nyota
IF x > 0:
    PRINT "OK"
```

### Błąd 5: tabulator

Tabulatory należy zamienić na dokładnie 4 spacje na poziom bloku. Interpreter nie uruchamia pliku z tabulacją.

### Błąd 6: małe litery w słowach kluczowych

```nyota
var x := 5
print x
```

Poprawka:

```nyota
VAR x := 5
PRINT x
```

### Błąd 7: zmiana typu

```nyota
VAR x := 5
x := "tekst"
```

Poprawka zależy od intencji. Można zmienić nazwę zmiennej albo wykonać konwersję.

### Błąd 8: RETURN w PROCEDURE

```nyota
PROCEDURE Test():
    RETURN 5
```

Poprawka: użyć `FUNCTION` albo usunąć `RETURN`.

### Błąd 9: FUNCTION bez RETURN

```nyota
FUNCTION Test():
    PRINT "Brak return"
```

Poprawka:

```nyota
FUNCTION Test():
    RETURN 0
```

### Błąd 10: `PRINT` po `GRAPH`

```nyota
GRAPH 6
PRINT "Tekst"
```

Powód: po wejściu w tryb graficzny `PRINT` powoduje twardy błąd interpretera.

### Błąd 11: nieznana instrukcja

```nyota
PRNIT "Hello"
```

Poprawka: poprawić nazwę instrukcji. Interpreter zgłasza `Nieznana instrukcja: PRNIT` zamiast cicho pomijać linię.

### Błąd 12: brak `BEGIN` / `END`

```nyota
PRINT "Hello"
```

Poprawka:

```nyota
BEGIN
PRINT "Hello"
END
```

To samo dotyczy pliku z `BEGIN` bez `END`, instrukcji przed `BEGIN` i instrukcji po `END`.

### Błąd 13: niezgodność typów w `+` albo `=`

```nyota
VAR x := 4 + "8"
IF 5 = 5.0:
    PRINT "zgaduje"
```

Poprawka:

```nyota
VAR x := 4 + INT("8")
IF FLT(5) = 5.0:
    PRINT "OK"
```

---

## 48. Format odpowiedzi agenta

Gdy agent analizuje kod Nyota, powinien odpowiadać według schematu:

```text
Problem:
<krótki opis problemu>

Przyczyna:
<dlaczego to jest błąd według zasad Nyota>

Poprawka:
<poprawiony fragment kodu>

Uwagi:
<opcjonalne ostrzeżenia, np. o typach, wcięciach, BEGIN/END>
```

---

## 49. Format patcha

Jeżeli AyoEdit obsługuje patch preview, agent powinien zwracać zmiany tak:

```diff
- IF x == 5:
+ IF x = 5:
       PRINT "OK"
```

Agent nie powinien przepisywać całego pliku, jeżeli wystarczy mała poprawka.

---

## 50. Minimalny prompt systemowy dla agenta AyoEdit

```text
Jesteś agentem programowania dla AyoEdit w systemie AyoOS.
Obsługujesz język Nyota oraz C.
Nyota jest autorskim językiem AyoOS.
Nie zakładaj składni innych języków.
Źródłem prawdy jest dokumentacja Nyota i błędy kompilatora/interpretera.
Słowa kluczowe Nyota pisz wielkimi literami.
Nie używaj średników, klamer blokowych, ==, != ani tabulatorów.
Bloki po dwukropku wcinaj dokładnie 4 spacjami.
Instrukcje wywołuj bez nawiasów, funkcje zwracające wartość z nawiasami.
Zmiany kodu proponuj jako patch albo krótki poprawiony fragment.
Jeżeli dokumentacja czegoś nie definiuje, nie zgaduj — napisz, że reguła nie jest określona.
```

---

## 51. Minimalny kontekst wysyłany do agenta przez AyoEdit

AyoEdit powinien przekazywać agentowi co najmniej:

```text
- język pliku: Nyota
- ścieżka pliku
- aktualnie otwarty plik albo zaznaczony fragment
- komunikaty kompilatora/interpretera
- ten dokument zasad
- opcjonalnie przykłady poprawnego kodu
```

Dla większych projektów AyoEdit powinien przekazywać także:

```text
- listę plików projektu
- importowane pliki .nyo
- definicje FUNCTION / PROCEDURE / RECORD
- wynik ostatniego uruchomienia
- wynik testów
```

---

## 52. Checklista agenta przed wygenerowaniem kodu Nyota

Przed odpowiedzią agent powinien sprawdzić:

* Czy program wykonywalny ma dokładnie jedną parę `BEGIN` / `END`? Interpreter odrzuca brak pary.
* Czy `IMPORT` jest przed `BEGIN`?
* Czy słowa kluczowe są wielkimi literami?
* Czy nie ma średników?
* Czy nie ma klamer blokowych?
* Czy nie ma tabulatorów?
* Czy bloki mają dokładnie 4 spacje?
* Czy nieznane instrukcje są zgłoszone jako błąd, a nie zignorowane?
* Czy użyto `:=` do przypisania?
* Czy użyto `=` zamiast `==`?
* Czy użyto `<>` albo `><` zamiast `!=`?
* Czy instrukcje są bez nawiasów?
* Czy funkcje zwracające wartość mają nawiasy?
* Czy `PROCEDURE` nie ma `RETURN`?
* Czy `FUNCTION` ma `RETURN` z wartością?
* Czy funkcja nie zwraca różnych typów?
* Czy typ zmiennej nie jest zmieniany po deklaracji?
* Czy `PRINT` i `GOTOXY` nie są użyte po `GRAPH`?
* Czy `MARK` ma zgodną liczbę kolumn?
* Czy `LEN()` jest użyty tylko na obsługiwanych typach?

---

## 53. Przykład pełnego poprawnego programu

```nyota
CONST MAX_LICZNIK := 5

PROCEDURE PokazStart():
    PRINT "Start programu Nyota"

FUNCTION Dodaj(a, b):
    RETURN a + b

BEGIN
PokazStart()
VAR wynik := Dodaj(2, 3)
PRINT "Wynik: " + STRING(wynik)

FOR i := 1 TO MAX_LICZNIK:
    PRINT "Krok: " + STRING(i)
END
```

---

## 54. Przykład programu graficznego

```nyota
BEGIN
GRAPH 6
CLEAR 0, 0, 0
BOX 100, 100, 300, 160, 4, 227, 138
LINE 100, 100, 400, 260, 255, 0, 85
CIRCLE 500, 300, 40, 0, 255, 240
END
```

Po `GRAPH` nie wolno używać `PRINT` ani `GOTOXY`.

---

## 55. Zasada bezpieczeństwa dla AyoEdit

Agent AI nie powinien bezpośrednio zapisywać zmian w plikach.

Bezpieczny przepływ:

```text
AI analizuje kod
AI proponuje patch
AyoEdit pokazuje podgląd
użytkownik zatwierdza
AyoEdit zapisuje
kompilator/interpreter Nyota sprawdza
błędy wracają do AI jako kontekst
```

Źródła prawdy:

1. specyfikacja Nyota,
2. kompilator/interpreter Nyota,
3. testy,
4. decyzja użytkownika.

---

## 56. Status dokumentu

Ten dokument jest bieżącą specyfikacją implementacji Nyoty v0.5 i ma odpowiadać kodowi interpretera oraz regresji.

<span style="color: #006A4E;">Dokument ponownie zsynchronizowano z `src/nyota.c`, `src/nyota_host.h`, backendem POSIX i testami 2026-09-19.</span>

Plan wydania i elementy przyszłe: [`docs/nyota_v05.md`](nyota_v05.md).

W kolejnych wersjach warto wydzielić osobne pliki:

```text
NYOTA_AGENT_RULES.md
NYOTA_SYNTAX.md
NYOTA_TYPES.md
NYOTA_STDLIB.md
NYOTA_GUI.md
NYOTA_GRAPHICS.md
NYOTA_ERRORS.md
NYOTA_EXAMPLES.md
```

Ten plik może pełnić rolę pliku głównego: `NYOTA_AGENT_RULES.md`.
