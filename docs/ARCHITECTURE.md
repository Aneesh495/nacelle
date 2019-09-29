# Nacelle Architecture Specification

Nacelle is a formally verified, high-assurance real-time event executive and deterministic telemetry engine engineered for mission-critical embedded platforms.

## Core Architectural Invariants

1. **Zero Dynamic Allocation in Execution Path**: All frame buffers, state machines, priority queues, and telemetry rings are statically allocated at startup. Once initialized, the flight executive performs zero calls to `malloc`, `free`, or any dynamic heap allocator.
2. **Formally Verified Protocol Boundaries**: Every packet header, framing projection, checksum computation, sliding-window state transition, and priority queue ordering is specified and mathematically verified in F*.
3. **Deterministic Cyclic Executive Scheduling**: Execution time is divided into a major cycle (1000 ms) partitioned into 10 harmonic minor cycles (100 ms each). Every task is statically assigned a minor period and an execution budget, guaranteeing bounded jitter and deadline compliance.

```
+---------------------------------------------------------------------------------------------------+
|                                      Nacelle Flight Executive                                     |
|                                                                                                   |
|  +--------------------+    +-----------------------+    +------------------+    +---------------+  |
|  | Telecommand Ingest | -> | Sequence State Machine| -> |  Priority Queue  | -> | Command Exec  |  |
|  |  (UDP Port 8088)   |    |    (FARM-1 / V(R))    |    |  (Min-Heap N=256)|    |  (Dispatches) |  |
|  +--------------------+    +-----------------------+    +------------------+    +---------------+  |
|                                                                                         |         |
|  +--------------------+    +-----------------------+    +------------------+            v         |
|  | Telemetry Downlink | <- | Telemetry Multiplexer | <- | Circular Journal | <- [Health Sensors]  |
|  |   (1024-Byte TM)   |    |  (VC Mux + Idle Fill) |    |  (1024-Byte Ring)|    [  FDIR Engine  ]  |
|  +--------------------+    +-----------------------+    +------------------+    +---------------+  |
+---------------------------------------------------------------------------------------------------+
```

## Subsystem Structure

### 1. Packet Processing Core (`src/fstar/Nacelle.Packet.*`, `src/c/src/packet.c`)

The packet layer implements fixed-overhead binary framing:
* **Primary Header (6 bytes)**:
  * Word 0 (16 bits): Version (3 bits), Kind (1 bit: 0=telemetry, 1=command), Secondary Header Flag (1 bit), Channel ID (11 bits).
  * Word 1 (16 bits): Sequence Grouping (2 bits), Sequence Counter (14 bits).
  * Word 2 (16 bits): Data Length Field (16 bits, representing total payload length minus 1).
* **Guarantees**: Total bounds checking. Rejection of truncated buffers (< 6 bytes or length < header data length). The round-trip theorem proves that serialization followed by parsing recovers the identical packet structure.

### 2. Sequence Validation & Link Control (`src/fstar/Nacelle.Sequence.*`, `src/c/src/sequence.c`)

Implements a sliding-window frame acceptance and reporting mechanism:
* **States**:
  * `STATE_OPEN`: Nominal frame reception. When a frame arrives with sequence number $N(S) == V(R)$, the receiver accepts the payload, increments $V(R) \pmod{256}$, and clears retransmit flags.
  * `STATE_WAIT`: Out-of-order frame detected within the positive window $W$. The receiver rejects the frame, flags retransmission, and waits for the missing sequence number.
  * `STATE_LOCKOUT`: Anomaly state triggered when receiving a sequence number outside the window. Rejects all sequenced frames until an explicit `FRAME_CONTROL_UNLOCK` command is ingested.
* **Link Status Reporting (OCF Word)**: Packed 32-bit status field containing Virtual Channel ID, Lockout Flag, Wait Flag, Retransmit Request Flag, and Report Value $V(R)$. Appended to every telemetry transfer frame.

### 3. Bounded Priority Min-Heap Queue (`src/fstar/Nacelle.Queue.*`, `src/c/src/queue.c`)

Commands scheduled for future execution or sorted by critical urgency are queued in a bounded binary min-heap:
* **Capacity**: Fixed $N = 256$ slots.
* **Ordering Predicate**: Total ordering based on (1) earliest dispatch timestamp, (2) severity priority level, and (3) command identifier tie-breaking.
* **Complexity**: $O(\log N)$ insertion and $O(\log N)$ extraction. In-place sift-up and sift-down without recursive stack growth.

### 4. Autonomous Fault Detection, Isolation, and Recovery (`src/fstar/Nacelle.FDIR.*`, `src/c/src/fdir.c`)

Continuous monitoring of telemetry channels (battery voltage, solar panel temperature, gyro rates):
* **Severity Classification**: Nominal, Warning, Critical based on static dual-band threshold limits.
* **Debounce Filter**: Sliding-window persistence counter. Transient spikes are filtered out; only anomalies persisting for $K$ consecutive evaluation cycles trip a recovery action.
* **Hierarchical Escalation**:
  * Tier 1: Component soft reset.
  * Tier 2: Switch to secondary redundant hardware channel.
  * Tier 3: Escalate to spacecraft safe-hold mode.
* **Absorbing Invariant**: Proved that escalation tiers are strictly monotonic and that safe-hold mode is absorbing.

### 5. Deterministic Cyclic Executive (`src/fstar/Nacelle.Executive.*`, `src/c/src/executive.c`)

The cyclic executive governs execution timing:
* **Major Cycle**: 1000 ms.
* **Minor Cycle**: 100 ms (10 minor frames per major frame).
* **Task Table**:
  * `tc_ingest`: Period 1 (10 Hz), budget 20 ms.
  * `cmd_dispatch`: Period 1 (10 Hz), budget 10 ms.
  * `health_monitor`: Period 2 (5 Hz), budget 15 ms.
  * `tm_downlink`: Period 5 (2 Hz), budget 30 ms.
* **Schedulability Theorem**: Total task budget in any minor cycle never exceeds 100 ms, leaving at least 25 ms of idle margin per cycle.
