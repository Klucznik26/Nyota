# Nyota

Niezależny język programowania.

- **Nyota** — język i interpreter
- **Tunga** — osobny edytor (nie jest częścią tego repozytorium)
- **AyoOS** — pierwszy system-host; tutaj rozwijamy host Linux

`PRINT`, `GRAPH`, `INPUT` i `DELAY` należą do języka i muszą działać na każdym hoście.

## Linux

Zależność: SDL2 (`pkg-config sdl2`).

```bash
make
./nyota tests/hello.nyo
make test
```

`GRAPH` otwiera okno SDL. Testy bez grafiki idą na stdout.

## Układ

```text
src/nyota.c          interpreter
src/nyota_host.h     kontrakt hosta
host/posix/          Linux: stdout + SDL
host/ayoos/          notatka o hoście AyoOS
docs/                specyfikacja i plan v0.5
tests/               programy .nyo
```

## Dokumenty

- `docs/nyota.md` — specyfikacja języka
- `docs/nyota_v05.md` — plan stabilizacji
- `docs/zasadymd.md` — jak oznaczać stan w `.md`
