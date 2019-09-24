# Nacelle Project Makefile
# Formally Verified Real-Time Event Executive and Telemetry Engine

CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc/c/include
FSTAR ?= /Users/aneeshkrishna/.local/fstar/bin/fstar.exe
PYTHON ?= python3

SRC_C = $(wildcard src/c/src/*.c)
OBJ_C = $(patsubst src/c/src/%.c,build/obj/%.o,$(SRC_C))

TEST_SRCS = $(wildcard tests/c/*.c)
TEST_BINS = $(patsubst tests/c/%.c,build/bin/%,$(TEST_SRCS))

.PHONY: all clean verify test test_c test_integration fuzz bench lint check_dashes

all: build/bin/nacelle_node $(TEST_BINS) build/bin/bench_throughput

build/obj:
	@mkdir -p build/obj build/bin

build/obj/%.o: src/c/src/%.c | build/obj
	$(CC) $(CFLAGS) -c $< -o $@

build/bin/nacelle_node: $(OBJ_C) | build/obj
	$(CC) $(CFLAGS) $(OBJ_C) -o $@

build/bin/test_checksum: tests/c/test_checksum.c build/obj/checksum.o | build/obj
	$(CC) $(CFLAGS) $< build/obj/checksum.o -o $@

build/bin/test_packet: tests/c/test_packet.c build/obj/packet.o | build/obj
	$(CC) $(CFLAGS) $< build/obj/packet.o -o $@

build/bin/test_sequence: tests/c/test_sequence.c build/obj/sequence.o | build/obj
	$(CC) $(CFLAGS) $< build/obj/sequence.o -o $@

build/bin/test_queue: tests/c/test_queue.c build/obj/queue.o | build/obj
	$(CC) $(CFLAGS) $< build/obj/queue.o -o $@

build/bin/test_fdir: tests/c/test_fdir.c build/obj/fdir.o | build/obj
	$(CC) $(CFLAGS) $< build/obj/fdir.o -o $@

build/bin/test_ring_buffer: tests/c/test_ring_buffer.c build/obj/ring_buffer.o | build/obj
	$(CC) $(CFLAGS) $< build/obj/ring_buffer.o -o $@

build/bin/test_executive: tests/c/test_executive.c build/obj/executive.o build/obj/queue.o build/obj/sequence.o build/obj/telemetry.o build/obj/fdir.o build/obj/ring_buffer.o build/obj/checksum.o | build/obj
	$(CC) $(CFLAGS) $^ -o $@

build/bin/test_integration: tests/c/test_integration.c build/obj/checksum.o build/obj/packet.o build/obj/sequence.o build/obj/telemetry.o build/obj/queue.o build/obj/fdir.o build/obj/ring_buffer.o build/obj/executive.o | build/obj
	$(CC) $(CFLAGS) $^ -o $@

build/bin/bench_throughput: tests/bench/bench_throughput.c build/obj/checksum.o build/obj/packet.o build/obj/queue.o | build/obj
	$(CC) $(CFLAGS) -O3 $^ -o $@

verify:
	./tools/run_fstar_verification.sh

test_c: $(TEST_BINS)
	@echo "Running C unit test suite..."
	@for t in $(TEST_BINS); do \
		echo "--> Running $$t ..."; \
		$$t; \
	done

test_integration: build/bin/nacelle_node
	$(PYTHON) tools/ground_station/scenario_runner.py

fuzz:
	$(PYTHON) tests/fuzz/fuzz_telecommand.py

bench: build/bin/bench_throughput
	./build/bin/bench_throughput

test: test_c test_integration fuzz

lint: check_dashes

check_dashes:
	$(PYTHON) tools/check_no_em_dashes.py

clean:
	rm -rf build
