module Nacelle.Timestamp

open Nacelle.Types
open Nacelle.Math
module U16 = FStar.UInt16
module U32 = FStar.UInt32
module U64 = FStar.UInt64

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

noeq type timestamp = {
  coarse_secs : u32;
  fine_ticks  : u16;
}

val timestamp_zero : timestamp
let timestamp_zero = {
  coarse_secs = 0ul;
  fine_ticks  = 0us;
}

val timestamp_compare : t1: timestamp -> t2: timestamp -> int
let timestamp_compare t1 t2 =
  if U32.v t1.coarse_secs < U32.v t2.coarse_secs then -1
  else if U32.v t1.coarse_secs > U32.v t2.coarse_secs then 1
  else if U16.v t1.fine_ticks < U16.v t2.fine_ticks then -1
  else if U16.v t1.fine_ticks > U16.v t2.fine_ticks then 1
  else 0

val timestamp_le : t1: timestamp -> t2: timestamp -> bool
let timestamp_le t1 t2 =
  (timestamp_compare t1 t2 <= 0)

val timestamp_lt : t1: timestamp -> t2: timestamp -> bool
let timestamp_lt t1 t2 =
  (timestamp_compare t1 t2 < 0)

val timestamp_add_ticks : t: timestamp -> ticks: u16 -> timestamp
let timestamp_add_ticks t ticks =
  let sum = U16.v t.fine_ticks + U16.v ticks in
  if sum >= 65536 then
    let carry = sum / 65536 in
    let new_fine = U16.uint_to_t (sum % 65536) in
    let new_coarse =
      if U32.v t.coarse_secs + carry < 4294967296 then
        U32.add t.coarse_secs (U32.uint_to_t carry)
      else 0xFFFFFFFFul
    in
    { coarse_secs = new_coarse; fine_ticks = new_fine }
  else
    { coarse_secs = t.coarse_secs; fine_ticks = U16.uint_to_t sum }

val lemma_timestamp_refl : t: timestamp -> Lemma (timestamp_compare t t == 0)
let lemma_timestamp_refl t = ()

val lemma_timestamp_antisym : t1: timestamp -> t2: timestamp ->
  Lemma (requires timestamp_compare t1 t2 == 0)
        (ensures t1.coarse_secs == t2.coarse_secs /\ t1.fine_ticks == t2.fine_ticks)
let lemma_timestamp_antisym t1 t2 = ()
