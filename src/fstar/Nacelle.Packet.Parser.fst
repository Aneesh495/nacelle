module Nacelle.Packet.Parser

open Nacelle.Types
open Nacelle.Math
open Nacelle.Packet.Header
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

noeq type packet = {
  header  : packet_header;
  payload : list u8;
}

val parse_header_bytes : b: list u8{List.Tot.length b >= 6} -> packet_header
let parse_header_bytes b =
  let b0 = List.Tot.index b 0 in
  let b1 = List.Tot.index b 1 in
  let b2 = List.Tot.index b 2 in
  let b3 = List.Tot.index b 3 in
  let b4 = List.Tot.index b 4 in
  let b5 = List.Tot.index b 5 in
  let w0 = make_u16_be b0 b1 in
  let w1 = make_u16_be b2 b3 in
  let w2 = make_u16_be b4 b5 in
  let (vers, k, sec, chan) = unpack_word0 w0 in
  let (grp, seq) = unpack_word1 w1 in
  {
    version        = vers;
    kind           = k;
    sec_header     = sec;
    channel        = chan;
    grouping       = grp;
    seq_count      = seq;
    data_len_field = w2;
  }

val take_bytes : n: nat -> s: list u8 -> Tot (list u8) (decreases n)
let rec take_bytes n s =
  if n = 0 then []
  else
    match s with
    | [] -> []
    | hd :: tl -> hd :: take_bytes (n - 1) tl

val drop_bytes : n: nat -> s: list u8 -> Tot (list u8) (decreases n)
let rec drop_bytes n s =
  if n = 0 then s
  else
    match s with
    | [] -> []
    | _ :: tl -> drop_bytes (n - 1) tl

val parse_packet : raw: list u8 -> option packet
let parse_packet raw =
  if List.Tot.length raw < 6 then None
  else
    let hdr = parse_header_bytes raw in
    let expected_payload_len = U16.v hdr.data_len_field + 1 in
    let total_expected = 6 + expected_payload_len in
    if List.Tot.length raw < total_expected then None
    else
      let after_hdr = drop_bytes 6 raw in
      let payload = take_bytes expected_payload_len after_hdr in
      Some { header = hdr; payload = payload }
