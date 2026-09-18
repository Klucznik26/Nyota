# NC / NZ / NA — plan modułów natywnych Nyoty

**Status:** PLAN  
**Data decyzji:** 2026-09-18  
**Zasada nadrzędna:** Nyota Platform Invariance Rule

## Cel

Nyota ma pozwolić programiście zejść do kodu natywnego bez utraty zasady,
że ten sam kod źródłowy programu działa bez zmian na AyoOS, Linuxie i Windowsie.

## Rodziny modułów

- **NC** — moduł napisany w C.
- **NZ** — moduł napisany w Zig.
- **NA** — moduł napisany w natywnym assemblerze CPU.
- **NYASM** — pozostaje osobno jako przenośna VM Nyoty; nie jest NA.

## Reguła przenośności

Kod Nyoty oraz źródła NC/NZ nie mogą zawierać różnic zależnych od systemu.
NA może być zależne od architektury CPU, ale dla tej samej architektury
źródło ma być identyczne na wszystkich hostach.

Niedozwolony model:

```c
#ifdef __linux__
...
#elif AYOOS
...
#elif _WIN32
...
#endif
```

Docelowy model:

```text
NC / NZ / NA
     |
     v
Nyota Native ABI + NyotaNativeAPI
     |
     +-- AyoOS adapter
     +-- Linux adapter
     +-- Windows adapter
```

## Toolchain

Różnice kompilatora są szczegółem narzędziowym.

```text
Linux    -> C/Zig toolchain dla targetu
Windows  -> C/Zig toolchain dla targetu
AyoOS    -> cross-compile lub przyszły toolchain natywny
```

Programista uruchamia ten sam projekt i nie zmienia źródeł między hostami.

## ABI

ABI ma używać typów o stałych rozmiarach. Nie należy wystawiać przez granicę
typów zależnych od platformy takich jak `long`, `size_t` czy `time_t`.

Pierwsza wersja powinna objąć przede wszystkim:
- INTEGER,
- BOOLEAN,
- FLOAT Nyoty w jej kanonicznej reprezentacji fixed-point,
- DATE,
- TIME,
- STRING przez jawny bufor + długość.

LIST, MARK i RECORD wymagają osobnego kontraktu uchwytów lub stabilnej
reprezentacji i nie powinny być zgadywane przed zamknięciem ABI.

## Kolejność wdrażania

1. Nyota Native ABI v1.
2. NyotaNativeAPI v1.
3. NC.
4. NZ.
5. NA dla x86-64.
6. Loader modułów na AyoOS, Linuxie i Windowsie.
7. Dopiero później rozszerzone typy i callbacki.

