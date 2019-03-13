module Nacelle.Packet.Header

open Nacelle.Types
open Nacelle.Math
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

type packet_kind =
  | KIND_TELEMETRY
  | KIND_COMMAND

let packet_kind_to_bool (k: packet_kind) : bool =
  match k with
  | KIND_TELEMETRY -> false
  | KIND_COMMAND   -> true

let bool_to_packet_kind (b: bool) : packet_kind =
  if b then KIND_COMMAND else KIND_TELEMETRY

type sequence_grouping =
  | GROUP_CONTINUATION
  | GROUP_FIRST
  | GROUP_LAST
  | GROUP_STANDALONE

let grouping_to_u8 (g: sequence_grouping) : u8 =
  match g with
  | GROUP_CONTINUATION -> 0uy
  | GROUP_FIRST        -> 1uy
  | GROUP_LAST         -> 2uy
  | GROUP_STANDALONE   -> 3uy

let u8_to_grouping (v: u8) : sequence_grouping =
  let m = U8.v v % 4 in
  if m = 0 then GROUP_CONTINUATION
  else if m = 1 then GROUP_FIRST
  else if m = 2 then GROUP_LAST
  else GROUP_STANDALONE

noeq type packet_header = {
  version        : u8;
  kind           : packet_kind;
  sec_header     : bool;
  channel        : channel_id;
  grouping       : sequence_grouping;
  seq_count      : sequence_id;
  data_len_field : u16;
}

val pack_word0 : h: packet_header -> u16
let pack_word0 h =
  let v3  = (U16.v (u8_to_u16 h.version) % 8) * 8192 in
  let typ = if packet_kind_to_bool h.kind then 4096 else 0 in
  let shf = if h.sec_header then 2048 else 0 in
  let apid = U16.v h.channel in
  U16.uint_to_t (v3 + typ + shf + apid)

val pack_word1 : h: packet_header -> u16
let pack_word1 h =
  let grp = (U8.v (grouping_to_u8 h.grouping) % 4) * 16384 in
  let seq = U16.v h.seq_count in
  U16.uint_to_t (grp + seq)

val unpack_word0 : w0: u16 -> (u8 & packet_kind & bool & channel_id)
let unpack_word0 w0 =
  let v_int = U16.v w0 in
  let vers = U8.uint_to_t ((v_int / 8192) % 8) in
  let k = bool_to_packet_kind (((v_int / 4096) % 2) = 1) in
  let sec = (((v_int / 2048) % 2) = 1) in
  let chan : channel_id = U16.uint_to_t (v_int % 2048) in
  (vers, k, sec, chan)

val unpack_word1 : w1: u16 -> (sequence_grouping & sequence_id)
let unpack_word1 w1 =
  let v_int = U16.v w1 in
  let grp = u8_to_grouping (U8.uint_to_t (v_int / 16384)) in
  let seq : sequence_id = U16.uint_to_t (v_int % 16384) in
  (grp, seq)
