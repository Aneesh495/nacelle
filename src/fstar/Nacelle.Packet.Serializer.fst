module Nacelle.Packet.Serializer

open Nacelle.Types
open Nacelle.Math
open Nacelle.Packet.Header
open Nacelle.Packet.Parser
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 7 --ifuel 2 --z3rlimit 30"

val serialize_header : h: packet_header -> list u8
let serialize_header h =
  let w0 = pack_word0 h in
  let w1 = pack_word1 h in
  let w2 = h.data_len_field in
  let (b0, b1) = split_u16_be w0 in
  let (b2, b3) = split_u16_be w1 in
  let (b4, b5) = split_u16_be w2 in
  [b0; b1; b2; b3; b4; b5]

val serialize_packet : p: packet -> list u8
let serialize_packet p =
  let hdr_bytes = serialize_header p.header in
  List.Tot.append hdr_bytes p.payload

val lemma_serialize_header_len : h: packet_header ->
  Lemma (List.Tot.length (serialize_header h) == 6)
let lemma_serialize_header_len h = ()

val lemma_serialize_packet_len : p: packet ->
  Lemma (List.Tot.length (serialize_packet p) == 6 + List.Tot.length p.payload)
let lemma_serialize_packet_len p =
  lemma_serialize_header_len p.header;
  List.Tot.append_length (serialize_header p.header) p.payload
