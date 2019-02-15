# Nacelle Project Makefile
# Deterministic High-Assurance Event Executive and Telemetry Engine

CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc/c/include
FSTAR ?= /Users/aneeshkrishna/.local/fstar/bin/fstar.exe
PYTHON ?= python3

SRC_C = $(wildcard src/c/src/*.c)
OBJ_C = $(patsubst src/c/src/%.c,build/obj/%.o,$(SRC_C))

.PHONY: all clean verify test test_c test_integration lint check_dashes bench

all: build/bin/nacelle_node

build/obj:
	@mkdir -p build/obj build/bin

build/obj/%.o: src/c/src/%.c | build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/bin/nacelle_node: $(OBJ_C) | build/obj
	$(CC) $(CFLAGS) $(OBJ_C) -o $@

verify:
	@echo "Running F* verification suite..."
	$(FSTAR) --odir build/fstar_cache --cache_checked_modules src/fstar/*.fst

lint: check_dashes

check_dashes:
	$(PYTHON) tools/check_no_em_dashes.py

clean:
	rm -rf build
