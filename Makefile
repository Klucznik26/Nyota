# Nyota — niezależny język. Ten Makefile buduje host POSIX (Linux).
CC      ?= gcc
CFLAGS  ?= -Wall -O2 -std=gnu11
PREFIX  ?= /usr
DESTDIR ?=
SDL_CFLAGS       := $(shell pkg-config --cflags sdl2)
SDL_LIBS         := $(shell pkg-config --libs sdl2)
SDL_IMAGE_CFLAGS := $(shell pkg-config --cflags SDL2_image)
SDL_IMAGE_LIBS   := $(shell pkg-config --libs SDL2_image)
SDL_TTF_CFLAGS   := $(shell pkg-config --cflags SDL2_ttf)
SDL_TTF_LIBS     := $(shell pkg-config --libs SDL2_ttf)
FONTCONFIG_CFLAGS := $(shell pkg-config --cflags fontconfig)
FONTCONFIG_LIBS   := $(shell pkg-config --libs fontconfig)

.PHONY: all clean test test-color test-graph install uninstall rpm

all: nyota

nyota: host/posix/host.c src/nyota.c src/nyota_host.h src/nyota_color.h host/posix/font8x8.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) $(SDL_IMAGE_CFLAGS) $(SDL_TTF_CFLAGS) $(FONTCONFIG_CFLAGS) -Isrc -Ihost/posix -o nyota host/posix/host.c $(SDL_LIBS) $(SDL_IMAGE_LIBS) $(SDL_TTF_LIBS) $(FONTCONFIG_LIBS) -lm

test: nyota test-color
	@bash scripts/run_tests.sh

test-color:
	@$(CC) $(CFLAGS) -Isrc -o tests/.color_palette_test tests/color_palette.c
	@tests/.color_palette_test
	@rm -f tests/.color_palette_test

test-graph: nyota
	@SDL_VIDEODRIVER=dummy NYOTA_NO_WAIT=1 ./nyota tests/graph_box.nyo
	@SDL_VIDEODRIVER=dummy NYOTA_NO_WAIT=1 ./nyota tests/graph_print_forbidden.nyo | grep -q BLAD

install: nyota
	install -d "$(DESTDIR)$(PREFIX)/bin"
	install -m 755 nyota "$(DESTDIR)$(PREFIX)/bin/nyota"
	install -d "$(DESTDIR)$(PREFIX)/share/nyota/tests"
	install -m 644 tests/*.nyo "$(DESTDIR)$(PREFIX)/share/nyota/tests/"
	install -d "$(DESTDIR)$(PREFIX)/share/nyota/tests/assets"
	install -m 644 tests/assets/* "$(DESTDIR)$(PREFIX)/share/nyota/tests/assets/"
	install -d "$(DESTDIR)$(PREFIX)/share/doc/nyota"
	install -m 644 README.md docs/*.md "$(DESTDIR)$(PREFIX)/share/doc/nyota/"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/bin/nyota"
	rm -rf "$(DESTDIR)$(PREFIX)/share/nyota"
	rm -rf "$(DESTDIR)$(PREFIX)/share/doc/nyota"

rpm:
	bash scripts/build-rpm.sh

clean:
	rm -f nyota tests/.color_palette_test
