# Nyota — niezależny język. Ten Makefile buduje host POSIX (Linux).
CC      ?= gcc
CFLAGS  ?= -Wall -O2 -std=gnu11
SDL_CFLAGS := $(shell pkg-config --cflags sdl2)
SDL_LIBS   := $(shell pkg-config --libs sdl2)

.PHONY: all clean test

all: nyota

nyota: host/posix/host.c src/nyota.c host/posix/ayo_api.h src/nyota_host.h
	$(CC) $(CFLAGS) $(SDL_CFLAGS) -Ihost/posix -o nyota host/posix/host.c $(SDL_LIBS)

test: nyota
	@bash scripts/run_tests.sh

clean:
	rm -f nyota
