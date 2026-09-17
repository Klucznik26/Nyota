# do_wdrożenia.md — Nyota

**Ostatnia weryfikacja całości:** 2026-09-17  
**Powiązane:** [`nyota_v05.md`](../../../docs/nyota_v05.md) — nadrzędny podział wdrożenia v0.5; [`nyota.md`](nyota.md) — specyfikacja dla agenta.

<span style="color: red;">Stara kolejka pomysłów. Nie wdrażać stąd SORT z algorytmami ani innych pozycji poza listą BLOCKS z nyota_v05.md zagrożenie 2026-09-17</span>

Po wdrożeniu każdej z funkcji powinna być ona usuwana z tego pliku i przenoszona do właściwego `.md` o Nyota.

Ten plik jest roboczą listą rzeczy do wdrożenia w języku Nyota. Nie jest pełną specyfikacją języka. Zawiera tylko elementy ustalone jako kierunki rozwoju, które mają czekać na implementację albo dokładniejsze rozpisanie.

---

# 1. SORT — sortowanie list

## Status

Do wdrożenia.

## Cel

Dodać do Nyoty prostą, czytelną instrukcję sortowania list.

`SORT` ma być instrukcją, ponieważ zmienia listę w miejscu. Zgodnie z zasadą Nyoty instrukcje wykonujące akcję są wywoływane bez nawiasów.

## Główna składnia

```nyota
SORT lista
SORT lista, ASC
SORT lista, DESC
SORT lista, REVERSE
SORT lista, BUBBLE
SORT lista, BUBBLE, ASC
SORT lista, BUBBLE, DESC
SORT lista, BUBBLE, REVERSE
SORT lista, QUICK, ASC
SORT lista, QUICK, DESC
```

## Kierunki sortowania

```text
ASC      — rosnąco, domyślnie
DESC     — malejąco, technicznie
REVERSE  — alias dla DESC, bardziej czytelny
```

## Znaczenie domyślne

```nyota
SORT lista
```

oznacza:

```text
sortuj listę rosnąco algorytmem domyślnym
```

Domyślny algorytm może być oznaczony jako:

```text
AUTO
```

czyli wybierany przez interpreter.

## Minimalny zestaw algorytmów

Nyota ma obsługiwać minimum 8 popularnych algorytmów sortowania:

```text
BUBBLE     — sortowanie bąbelkowe
INSERT     — sortowanie przez wstawianie
SELECT     — sortowanie przez wybór
MERGE      — sortowanie przez scalanie
QUICK      — sortowanie szybkie
HEAP       — sortowanie kopcowe
SHELL      — sortowanie Shella
COUNTING   — sortowanie przez zliczanie
```

Dodatkowo:

```text
AUTO       — algorytm domyślny wybierany przez interpreter
```

## Zasady typów

Do doprecyzowania podczas implementacji.

Proponowana zasada robocza:

```text
SORT działa tylko na listach, których elementy są porównywalne.
Lista mieszanych typów powinna powodować błąd interpretera.
```

Na start najlepiej obsłużyć:

```text
INTEGER
STRING
BOOLEAN
```

Później można rozszerzyć o:

```text
FLOAT
RECORD przez SORT BY
```

## Możliwy etap późniejszy

Funkcja zwracająca posortowaną kopię:

```nyota
VAR nowa := SORTED(lista)
VAR nowa := SORTED(lista, QUICK, DESC)
```

To nie jest priorytet. Najpierw wdrożyć instrukcję `SORT`.

---

# 2. ASM / NYASM — kontrolowany assembler w Nyocie

## Status

Do zaprojektowania i późniejszego wdrożenia. Nie wdrażać przed ustabilizowaniem rdzenia Nyoty.

## Cel

Dodać możliwość użycia asemblera w Nyocie, ale jako kontrolowany tryb ekspercki, bez łamania zasad bezpieczeństwa języka.

## Preferowany kierunek

Najpierw wdrożyć bezpieczny wirtualny assembler Nyoty:

```text
NYASM
```

Dopiero później rozważyć natywny assembler x86-64:

```text
NATIVE ASM
XASM
```

## Preferowana składnia

```nyota
ASM INPUT a, b OUTPUT wynik:
       MOV R0, a
       ADD R0, b
       STORE wynik, R0
```

Blok `ASM` ma być zgodny z zasadami bloków Nyoty:

```text
linia kończy się dwukropkiem
wnętrze bloku ma dokładnie 7 spacji wcięcia
powrót do wcześniejszego wcięcia kończy blok
```

## Jawne wejścia i wyjścia

ASM musi jawnie deklarować, z jakich zmiennych czyta i do jakich zapisuje:

```nyota
ASM INPUT x, y OUTPUT suma:
       MOV R0, x
       ADD R0, y
       STORE suma, R0
```

Nie wolno dopuścić, aby ASM ukrycie modyfikował dowolne zmienne Nyoty.

## Rejestry NYASM

Proponowane rejestry wirtualne:

```text
R0
R1
R2
R3
```

Na start wystarczą cztery rejestry.

## Obsługiwane typy na start

Na start ASM powinien obsługiwać głównie:

```text
INTEGER
BOOLEAN jako 0/1
```

Później można rozważyć:

```text
FLOAT fixed-point
BANK
kontrolowane bufory
```

## Twarde ograniczenia

ASM w Nyocie nie może:

```text
zmieniać typu zmiennej
alokować pamięci Nyoty
mieszać w stosie interpretera
skakać do linii Nyoty
wywoływać RETURN
wywoływać EXIT
wywoływać PROCEDURE/FUNCTION bez kontroli
modyfikować STRING/LIST bez specjalnego trybu
wywoływać syscalli AyoOS bez pozwolenia
```

## Etapy wdrażania

```text
1. NYASM jako wirtualny assembler bezpieczny.
2. ASM INPUT/OUTPUT z jawną listą zmiennych.
3. Dopiero później NATIVE ASM / XASM jako tryb ekspercki.
```

---

# 3. FILE — bezpieczne operacje plikowe wysokiego poziomu

## Status

Do zaprojektowania. Zapamiętane jako jeden z najbliższych kierunków integracji Nyoty z AyoOS.

## Cel

Dodać do Nyoty bezpieczne operacje plikowe wysokiego poziomu, działające przez AyoAPI / VFS, bez bezpośredniego dostępu do struktur jądra.

## Zasada ogólna

```text
Nyota -> AyoAPI -> VFS / kernel
```

Nie:

```text
Nyota -> bezpośrednie struktury FS / kernel
```

## Wstępny podział

Instrukcje wykonujące akcję — bez nawiasów:

```nyota
FILE_WRITE "A:/test.txt", "Witaj"
FILE_APPEND "A:/test.txt", "Nowa linia"
FILE_DELETE "A:/test.txt"
FILE_COPY "A:/test.txt", "A:/kopia.txt"
FILE_MOVE "A:/kopia.txt", "A:/folder/kopia.txt"
```

Funkcje zwracające wartość — z nawiasami:

```nyota
VAR tekst := FILE_READ("A:/test.txt")
VAR istnieje := FILE_EXISTS("A:/test.txt")
VAR rozmiar := FILE_SIZE("A:/test.txt")
```

## Uwagi

Na start najlepiej wdrożyć proste operacje całościowe.

Nie wdrażać od razu pełnego modelu uchwytów:

```text
OPEN
CLOSE
READLINE
WRITEHANDLE
SEEK
```

Uchwyty plików można dodać później jako etap bardziej zaawansowany.

---

# 4. DIR / LS — katalogi i listowanie

## Status

Do zaprojektowania. Zapamiętane jako drugi najbliższy kierunek integracji Nyoty z AyoOS.

## Cel

Dodać do Nyoty podstawowe operacje katalogowe i listowanie katalogów.

## Proponowane instrukcje katalogowe

```nyota
DIR_CREATE "A:/Nowy"
DIR_DELETE "A:/Nowy"
DIR_COPY "A:/Stary", "A:/Nowy"
DIR_MOVE "A:/Nowy", "A:/Inny"
```

## Proponowane funkcje katalogowe

```nyota
VAR jest := DIR_EXISTS("A:/System")
VAR pliki := DIR_LIST("A:/System")
```

## DIR_LIST()

Na start `DIR_LIST()` powinno zwracać prostą listę nazw jako `LIST` stringów:

```nyota
VAR pliki := DIR_LIST("A:/System")
PRINT pliki[0]
```

Później można dodać bogatszą wersję:

```nyota
VAR wpisy := DIR_LIST_EX("A:/System")
```

gdzie każdy wpis może być rekordem z polami:

```text
name
type
size
modified
```

## LS

Do rozważenia jako krótsza forma listowania, bardziej shellowa:

```nyota
VAR pliki := LS("A:/System")
```

albo jako instrukcja wypisująca katalog na ekran:

```nyota
LS "A:/System"
```

Trzeba zdecydować, czy `LS` ma być:

```text
funkcją zwracającą LIST
czy instrukcją wypisującą zawartość katalogu
```

Zgodnie z zasadami Nyoty:

```text
jeśli zwraca listę -> LS(...)
jeśli tylko wypisuje -> LS "A:/System"
```

---

# Kolejność robocza

Proponowana kolejność dalszego dopracowania:

```text
1. DIR/LS — zdecydować, czy LS jest funkcją, instrukcją, czy obie formy.
2. FILE — ustalić minimalny zestaw operacji całościowych.
3. SORT — rozpisać dokładną specyfikację błędów i typów.
4. ASM/NYASM — trzymać jako późniejszy tryb ekspercki.
```

---

# Zasada przenoszenia do dokumentacji głównej

Po wdrożeniu funkcji należy:

```text
1. Usunąć jej opis z tego pliku.
2. Przenieść ostateczną składnię do właściwego pliku dokumentacji Nyoty.
3. Dopisać przykłady poprawnego kodu.
4. Dopisać przykłady błędnego kodu.
5. Dopisać zasady dla agenta AI.
6. Dopisać testy interpretera.
```
