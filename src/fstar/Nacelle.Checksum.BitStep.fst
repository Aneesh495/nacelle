module Nacelle.Checksum.BitStep

open Nacelle.Types
open Nacelle.Math
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

let crc16_poly : u16 = 0x1021us
let crc16_init : u16 = 0xFFFFus

val crc16_step_bit : crc: u16 -> bit: bool -> u16
let crc16_step_bit crc bit =
  let msb = (U16.v (U16.shift_right crc 15ul) = 1) in
  let shifted = U16.shift_left crc 1ul in
  let with_bit = if bit then U16.logor shifted 1us else shifted in
  if msb then U16.logxor with_bit crc16_poly
  else with_bit

val crc16_step_byte : crc: u16 -> b: u8 -> u16
let crc16_step_byte crc b =
  let rec step (c: u16) (i: nat{i <= 8}) : Tot u16 (decreases i) =
    if i = 0 then c
    else
      let bit_idx = U32.uint_to_t (8 - i) in
      let bit_val = (U8.v (U8.logand (U8.shift_right b bit_idx) 1uy) = 1) in
      let c' = crc16_step_bit c bit_val in
      step c' (i - 1)
  in
  step crc 8

val crc16_compute_bits : s: list u8 -> crc: u16 -> Tot u16 (decreases s)
let rec crc16_compute_bits s crc =
  match s with
  | [] -> crc
  | hd :: tl ->
      let next_crc = crc16_step_byte crc hd in
      crc16_compute_bits tl next_crc

val crc16_bytes_bits : s: list u8 -> u16
let crc16_bytes_bits s = crc16_compute_bits s crc16_init
