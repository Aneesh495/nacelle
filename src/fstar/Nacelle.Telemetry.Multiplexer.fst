module Nacelle.Telemetry.Multiplexer

open Nacelle.Types
open Nacelle.Math
open Nacelle.Telemetry.Types
module U8  = FStar.UInt8
module U16 = FStar.UInt16

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

let fhp_no_first_header : u16 = 0x07FEus
let fhp_idle_data_only  : u16 = 0x07FFus

val is_valid_fhp : fhp: u16 -> max_payload: nat -> bool
let is_valid_fhp fhp max_payload =
  (U16.v fhp < max_payload) ||
  (fhp = fhp_no_first_header) ||
  (fhp = fhp_idle_data_only)

val compute_next_fhp :
  remaining_in_frame: nat ->
  packet_bytes_left: nat ->
  Tot u16
let compute_next_fhp remaining_in_frame packet_bytes_left =
  if remaining_in_frame = 0 then fhp_no_first_header
  else if packet_bytes_left >= remaining_in_frame then fhp_no_first_header
  else
    if packet_bytes_left < 2048 then
      U16.uint_to_t packet_bytes_left
    else fhp_no_first_header

val lemma_fhp_bounded :
  remaining: nat ->
  bytes_left: nat ->
  Lemma (requires remaining <= 1012 /\ bytes_left < remaining)
        (ensures U16.v (compute_next_fhp remaining bytes_left) < remaining)
let lemma_fhp_bounded remaining bytes_left = ()
