module Nacelle.Packet.Lemmas

open Nacelle.Types
open Nacelle.Math
open Nacelle.Packet.Header
open Nacelle.Packet.Parser
open Nacelle.Packet.Serializer
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 7 --ifuel 2 --z3rlimit 40"

let packet_well_formed (p: packet) : bool =
  (U8.v p.header.version < 8) &&
  (List.Tot.length p.payload > 0) &&
  (List.Tot.length p.payload <= 65536) &&
  (U16.v p.header.data_len_field = List.Tot.length p.payload - 1)

val lemma_word0_roundtrip : w0: u16 ->
  Lemma (let (v, k, s, c) = unpack_word0 w0 in
         let h = { version = v; kind = k; sec_header = s; channel = c;
                   grouping = GROUP_STANDALONE; seq_count = 0us; data_len_field = 0us } in
         pack_word0 h == w0)
let lemma_word0_roundtrip w0 = ()

val lemma_word1_roundtrip : w1: u16 ->
  Lemma (let (g, seq) = unpack_word1 w1 in
         let h = { version = 0uy; kind = KIND_TELEMETRY; sec_header = false; channel = 0us;
                   grouping = g; seq_count = seq; data_len_field = 0us } in
         pack_word1 h == w1)
let lemma_word1_roundtrip w1 = ()

val lemma_header_len_correct : p: packet ->
  Lemma (List.Tot.length (serialize_header p.header) == 6)
let lemma_header_len_correct p =
  lemma_serialize_header_len p.header

val lemma_serialized_packet_bounds : p: packet ->
  Lemma (requires packet_well_formed p)
        (ensures List.Tot.length (serialize_packet p) >= 7)
let lemma_serialized_packet_bounds p =
  lemma_serialize_packet_len p
