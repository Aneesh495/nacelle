module Nacelle.Telemetry.Idle

open Nacelle.Types
open Nacelle.Math
open Nacelle.Packet.Header
module U8  = FStar.UInt8
module U16 = FStar.UInt16

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

let idle_channel_id : channel_id = 0x07FFus

val make_idle_byte : idx: nat -> u8
let make_idle_byte idx =
  if idx % 2 = 0 then 0x55uy else 0xAAuy

val generate_idle_bytes : count: nat -> Tot (list u8) (decreases count)
let rec generate_idle_bytes count =
  if count = 0 then []
  else
    make_idle_byte count :: generate_idle_bytes (count - 1)

val lemma_idle_bytes_length : count: nat ->
  Lemma (List.Tot.length (generate_idle_bytes count) == count)
let rec lemma_idle_bytes_length count =
  if count = 0 then ()
  else lemma_idle_bytes_length (count - 1)

val make_idle_header : length_minus_1: u16 -> packet_header
let make_idle_header length_minus_1 = {
  version        = 0uy;
  kind           = KIND_TELEMETRY;
  sec_header     = false;
  channel        = idle_channel_id;
  grouping       = GROUP_STANDALONE;
  seq_count      = 0us;
  data_len_field = length_minus_1;
}
