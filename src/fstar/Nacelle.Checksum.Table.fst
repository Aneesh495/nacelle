module Nacelle.Checksum.Table

open Nacelle.Types
open Nacelle.Math
open Nacelle.Checksum.BitStep
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

val table_entry_for_byte : b: u8 -> u16
let table_entry_for_byte b =
  let w = U16.shift_left (u8_to_u16 b) 8ul in
  let rec step (c: u16) (i: nat{i <= 8}) : Tot u16 (decreases i) =
    if i = 0 then c
    else
      let msb = (U16.v (U16.shift_right c 15ul) = 1) in
      let c' = U16.shift_left c 1ul in
      let c'' = if msb then U16.logxor c' crc16_poly else c' in
      step c'' (i - 1)
  in
  step w 8

val crc16_step_fast : crc: u16 -> b: u8 -> u16
let crc16_step_fast crc b =
  let top = U8.uint_to_t (U16.v (U16.shift_right crc 8ul)) in
  let idx = U8.logxor top b in
  let entry = table_entry_for_byte idx in
  let shifted = U16.shift_left crc 8ul in
  U16.logxor shifted entry

val crc16_compute_fast : s: list u8 -> crc: u16 -> Tot u16 (decreases s)
let rec crc16_compute_fast s crc =
  match s with
  | [] -> crc
  | hd :: tl ->
      let next_crc = crc16_step_fast crc hd in
      crc16_compute_fast tl next_crc

val crc16_bytes_fast : s: list u8 -> u16
let crc16_bytes_fast s = crc16_compute_fast s crc16_init

val lemma_crc_empty : crc: u16 -> Lemma (crc16_compute_fast [] crc == crc)
let lemma_crc_empty crc = ()
