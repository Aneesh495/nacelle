# Nacelle Formal Verification Guide

This guide details the mathematical specifications, inductive proofs, and verification methodology used throughout the 25 F* modules in Nacelle.

## Verified Modules Catalog

```
src/fstar/
  Nacelle.Types.fst                 Bounded integer aliases, status codes, sequence lengths
  Nacelle.Math.fst                  Endianness bit packing, byte shifts, range proofs
  Nacelle.Checksum.BitStep.fst      Polynomial bit-level stepping specification
  Nacelle.Checksum.Table.fst        Table-driven accelerated checksum stepping
  Nacelle.Timestamp.fst             Monotonic flight clock arithmetic and total order
  Nacelle.Packet.Header.fst         Primary header bit-field layout and projection
  Nacelle.Packet.Parser.fst         Bounds-checked verified packet parser
  Nacelle.Packet.Serializer.fst     Verified serializer with exact length preservation
  Nacelle.Packet.Lemmas.fst         Decomposition roundtrip and boundary properties
  Nacelle.Sequence.Types.fst        Sliding window receiver state space and window predicates
  Nacelle.Sequence.State.fst        Deterministic sliding window state transition function
  Nacelle.Sequence.Lemmas.fst       Strict in-order acceptance and lockout safety theorems
  Nacelle.Sequence.Report.fst       Link status report word generation and packing logic
  Nacelle.Telemetry.Types.fst       Transfer frame structures and header bit packing
  Nacelle.Telemetry.Multiplexer.fst First header pointer calculation and segmentation lemmas
  Nacelle.Telemetry.Idle.fst        Deterministic idle padding generator and length lemmas
  Nacelle.Queue.Types.fst           Priority queue entries and total ordering relations
  Nacelle.Queue.Heap.fst            Bounded priority queue and sorted insertion
  Nacelle.Queue.Lemmas.fst          Capacity bounds and peek-extract equivalence lemmas
  Nacelle.FDIR.Types.fst            Health channel thresholds and severity classification
  Nacelle.FDIR.Filter.fst           Persistence debounce filtering and transient suppression
  Nacelle.FDIR.Lemmas.fst           Monotonicity, bound preservation, and absorbing safe hold
  Nacelle.Executive.Types.fst       Cyclic executive major and minor cycle task descriptors
  Nacelle.Executive.Scheduler.fst   Cycle advancement and minor frame budget schedulability
  Nacelle.Executive.Lemmas.fst      Cycle bounds and dispatch monotonicity theorems
```

## Key Verification Theorems

### 1. Packet Round-Trip Bijectivity
In `Nacelle.Packet.Lemmas.fst`:
```fstar
val lemma_word0_roundtrip : w0: u16 ->
  Lemma (let (v, k, s, c) = unpack_word0 w0 in
         let h = { version = v; kind = k; sec_header = s; channel = c;
                   grouping = GROUP_STANDALONE; seq_count = 0us; data_len_field = 0us } in
         pack_word0 h == w0)

val lemma_word1_roundtrip : w1: u16 ->
  Lemma (let (g, seq) = unpack_word1 w1 in
         let h = { version = 0uy; kind = KIND_TELEMETRY; sec_header = false; channel = 0us;
                   grouping = g; seq_count = seq; data_len_field = 0us } in
         pack_word1 h == w1)
```
Proves that word unpacking followed by packing is an identity transformation for all valid 16-bit header bitfields.

### 2. Strict In-Order Sequence Acceptance
In `Nacelle.Sequence.Lemmas.fst`:
```fstar
val lemma_accepted_sequenced_frame_exact :
  status: receiver_status ->
  frame: frame_header ->
  Lemma (requires status.state == STATE_OPEN /\
                  frame.category == FRAME_SEQUENCED /\
                  snd (process_frame status frame) == true)
        (ensures frame.frame_seq_num == status.expected_seq /\
                 (fst (process_frame status frame)).expected_seq == next_seq status.expected_seq)
```
Proves that when the receiver is in `STATE_OPEN`, a sequenced frame is accepted if and only if its sequence number matches the expected sequence number $V(R)$, and that $V(R)$ increments monotonically by 1 modulo 256.

### 3. Lockout Safety
In `Nacelle.Sequence.Lemmas.fst`:
```fstar
val lemma_lockout_rejects_sequenced :
  status: receiver_status ->
  frame: frame_header ->
  Lemma (requires status.state == STATE_LOCKOUT /\
                  frame.category == FRAME_SEQUENCED)
        (ensures snd (process_frame status frame) == false /\
                 (fst (process_frame status frame)).state == STATE_LOCKOUT)
```
Guarantees that once in lockout, zero sequenced frames can be accepted until an authenticated `FRAME_CONTROL_UNLOCK` is received.

### 4. FDIR Monotonicity and Absorbing Safe Hold
In `Nacelle.FDIR.Lemmas.fst`:
```fstar
val lemma_tier_monotonic :
  state: monitor_channel_state ->
  sample: u16 ->
  thresh: channel_thresholds ->
  Lemma (U8.v (fst (update_channel state sample thresh)).escalation_tier >= U8.v state.escalation_tier)

val lemma_safe_hold_absorbing :
  state: monitor_channel_state ->
  sample: u16 ->
  thresh: channel_thresholds ->
  Lemma (requires U8.v state.escalation_tier == 3)
        (ensures U8.v (fst (update_channel state sample thresh)).escalation_tier == 3)
```
Proves that recovery escalation is strictly acyclic, bounded by tier 3, and that safe-hold mode is absorbing.

## Running Verification

To execute the full verification suite across all 25 modules:
```bash
make verify
```
or directly via the verification script:
```bash
./tools/run_fstar_verification.sh
```
All verification conditions are discharged using Z3.
