module Nacelle.Math

open Nacelle.Types
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32
module U64 = FStar.UInt64

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

val u8_to_u16 : u8 -> u16
let u8_to_u16 x = U16.uint_to_t (U8.v x)

val u8_to_u32 : u8 -> u32
let u8_to_u32 x = U32.uint_to_t (U8.v x)

val u16_to_u32 : u16 -> u32
let u16_to_u32 x = U32.uint_to_t (U16.v x)

val make_u16_be : hi: u8 -> lo: u8 -> u16
let make_u16_be hi lo =
  let h16 = U16.shift_left (u8_to_u16 hi) 8ul in
  let l16 = u8_to_u16 lo in
  U16.logor h16 l16

val split_u16_be : w: u16 -> (u8 & u8)
let split_u16_be w =
  let hi_int = U16.v w / 256 in
  let lo_int = U16.v w % 256 in
  (U8.uint_to_t hi_int, U8.uint_to_t lo_int)

val make_u32_be : b0: u8 -> b1: u8 -> b2: u8 -> b3: u8 -> u32
let make_u32_be b0 b1 b2 b3 =
  let s0 = U32.shift_left (u8_to_u32 b0) 24ul in
  let s1 = U32.shift_left (u8_to_u32 b1) 16ul in
  let s2 = U32.shift_left (u8_to_u32 b2) 8ul in
  let s3 = u8_to_u32 b3 in
  U32.logor (U32.logor s0 s1) (U32.logor s2 s3)

val clamp_u16 : v: u32 -> max_val: u16 -> u16
let clamp_u16 v max_val =
  if U32.v v > U16.v max_val then max_val
  else U16.uint_to_t (U32.v v)

val lemma_u8_bounds : x: u8 -> Lemma (U8.v x >= 0 /\ U8.v x < 256)
let lemma_u8_bounds x = ()

val lemma_u16_bounds : x: u16 -> Lemma (U16.v x >= 0 /\ U16.v x < 65536)
let lemma_u16_bounds x = ()

val lemma_u32_bounds : x: u32 -> Lemma (U32.v x >= 0 /\ U32.v x < 4294967296)
let lemma_u32_bounds x = ()
