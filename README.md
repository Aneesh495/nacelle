# Nacelle

A formally verified real-time event executive and deterministic telemetry engine engineered in F* and C11 for high-assurance embedded platforms.

```
                      +-----------------------------+
                      |   Nacelle Flight Software   |
                      +-----------------------------+
                                     |
         +---------------------------+---------------------------+
         |                           |                           |
         v                           v                           v
+------------------+       +-------------------+       +------------------+
|  Formal Core     |       |  Deterministic    |       |  Ground Station  |
|  (25 F* Modules) |       |  Runtime (C11)    |       |  Control Console |
|                  |       |                   |       |                  |
| - Roundtrip Spec |       | - Zero Malloc     |       | - Live Telemetry |
| - CRC Theorems   |       | - Cyclic 100ms    |       | - COP-1 Sender   |
| - In-Order FARM  |       | - Bounded Queue   |       | - Auto Scenarios |
| - Acyclic FDIR   |       | - Ring Journal    |       | - Mutator Fuzzer |
+------------------+       +-------------------+       +------------------+
```

## Overview

In mission-critical embedded software, unhandled out-of-bounds reads, corrupted sequence counters, memory allocator fragmentation, and non-deterministic scheduling can lead to catastrophic system loss.

Nacelle addresses these failure modes by formally specifying and mathematically proving the core protocol and executive state machines in F*, and extracting/implementing a zero-allocation, hard-real-time C11 flight kernel.

### Architectural Highlights

* **Formally Verified in F***: 25 standalone specification and proof modules verified with Z3, establishing packet bijectivity, sliding-window in-order delivery, lockout safety, and fault recovery termination.
* **Deterministic Cyclic Executive**: Dual-tiered major cycle (1000 ms) and minor cycle (100 ms) harmonic scheduler with statically partitioned execution budgets.
* **Zero Dynamic Allocation**: Zero calls to `malloc`, `free`, or dynamic heap routines in the flight loop. All priority queues, ring buffers, and frame descriptors are statically sized.
* **Sliding-Window Sequence Control**: Implements positive and negative acceptance windows, automated retransmission signaling via Operational Control Fields, and lockout containment.
* **Autonomous Multi-Tier FDIR**: Multi-channel sensor monitoring with sliding-window debounce filters and acyclic three-tier fault escalation (Soft Reset, Secondary Switch, Safe-Hold Mode).
* **High Performance**:
  * CRC-16 Throughput: **325+ MB/s**
  * Packet Parsing and Serialization: **146+ Million packets/sec** (6.8 ns per packet)
  * Priority Heap Operations: **43+ Million ops/sec**

## Repository Structure

```
.
├── Makefile                           Build harness for C, F*, tests, and benchmarks
├── src/
│   ├── fstar/                         25 formally verified F* modules
│   │   ├── Nacelle.Types.fst          Bounded type definitions and status codes
│   │   ├── Nacelle.Math.fst           Bitwise lemmas and endianness conversions
│   │   ├── Nacelle.Checksum.*.fst     Polynomial stepper and table equivalence
│   │   ├── Nacelle.Timestamp.fst      Monotonic flight clock arithmetic
│   │   ├── Nacelle.Packet.*.fst       Primary header, bounds parser, and bijectivity
│   │   ├── Nacelle.Sequence.*.fst     Sliding-window state machine and theorems
│   │   ├── Nacelle.Telemetry.*.fst    Frame multiplexer and idle padding
│   │   ├── Nacelle.Queue.*.fst        Bounded min-heap priority queue
│   │   ├── Nacelle.FDIR.*.fst         Threshold monitoring and recovery escalation
│   │   └── Nacelle.Executive.*.fst    Harmonic cyclic executive scheduler
│   └── c/
│       ├── include/nacelle/           Public C11 subsystem headers
│       └── src/                       C11 flight kernel implementation
├── tools/
│   ├── check_no_em_dashes.py          Automated punctuation and formatting linter
│   ├── run_fstar_verification.sh      Automated 25-module F* verification runner
│   └── ground_station/                Python ground control console and protocols
│       ├── protocol.py                Uplink encoder and telemetry frame decoder
│       ├── ground_station.py          Interactive CLI command console and monitor
│       └── scenario_runner.py         Automated flight scenario test executor
├── tests/
│   ├── c/                             C unit tests and integration pipeline
│   ├── fuzz/                          Automated bitflip and mutation fuzzer
│   └── bench/                         Nanosecond throughput and latency benchmark
└── docs/
    ├── ARCHITECTURE.md                System architecture and timing specifications
    ├── VERIFICATION_GUIDE.md          F* theorem guide and proof documentation
    └── MISSION_OPERATIONS.md          Telecommand dictionary and telemetry guide
```

## Quickstart

### Prerequisites

* Clang or GCC supporting C11 (`-std=c11`)
* Python 3.8+
* (Optional) F* compiler (`fstar.exe`) for running formal verification proofs

### 1. Build the Flight Node and Test Suite

```bash
make clean
make all
```

This compiles `build/bin/nacelle_node`, the C unit test suite, and the throughput benchmark.

### 2. Run the Full Test Suite

```bash
make test
```

This executes:
1. All C unit tests (checksum, packet parser, sequence state machine, priority queue, FDIR engine, ring buffer, cyclic executive).
2. End-to-end integration pipeline test.
3. Automated mission scenario runner with socket-level flight node execution.
4. 5000-iteration telecommand parser mutation and bitflip fuzzer.

### 3. Run Formal Verification

To verify all 25 F* modules and discharge all inductive proof obligations using Z3:

```bash
make verify
```

### 4. Run the Flight Node and Ground Station Console

Start the flight node on UDP port 8088:
```bash
./build/bin/nacelle_node --port 8088
```

In a separate terminal, launch the live telemetry monitor:
```bash
python3 tools/ground_station/ground_station.py --port 8088 --monitor
```

Dispatch an uplink telecommand:
```bash
python3 tools/ground_station/ground_station.py --port 8088 --cmd-reset
```

### 5. Run Performance Benchmarks

```bash
make bench
```

## Formally Proved Properties

* **Packet Bijectivity**: Parsing a serialized well-formed packet recovers the identical packet structure without memory leaks or buffer out-of-bounds reads.
* **Strict Monotonicity**: Under arbitrary out-of-order frame arrivals, the sequence receiver accepts sequenced frames strictly monotonically without dropped or duplicated dispatches.
* **Lockout Containment**: Corrupted sequence frames outside the positive acceptance window safely force the receiver into lockout until an authenticated control unlock frame is received.
* **FDIR Termination**: Fault mitigation escalation is guaranteed to be strictly monotonic and bounded by tier 3, preventing cyclic oscillations between recovery states.
* **Harmonic Schedulability**: Major and minor frame periods guarantee that all periodic tasks complete within their allotted execution budget.

## Documentation

* [Architecture Specification](docs/ARCHITECTURE.md)
* [Formal Verification Guide](docs/VERIFICATION_GUIDE.md)
* [Mission Operations Manual](docs/MISSION_OPERATIONS.md)

## License

This project is licensed under the Apache License 2.0.
