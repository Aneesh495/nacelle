module Nacelle.Telemetry.Types

open Nacelle.Types
open Nacelle.Math
open Nacelle.Sequence.Report
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

let telemetry_frame_size : nat = 1024
let telemetry_header_size : nat = 6
let telemetry_trailer_size : nat = 6 (* 4-byte report + 2-byte CRC *)
let telemetry_payload_size : nat = 1012

type vcid_t = id: u8{U8.v id < 64}

noeq type telemetry_header = {
  version_num      : u8;
  spacecraft_id    : u16;
  virtual_channel  : vcid_t;
  ocf_flag         : bool;
  master_frame_seq : u8;
  virt_frame_seq   : u8;
  first_header_ptr : u16;
}

noeq type telemetry_frame = {
  header     : telemetry_header;
  data_field : bounded_byte_seq 1012;
  link_ctrl  : link_report;
  fecf_crc   : u16;
}

val pack_tm_word0 : h: telemetry_header -> u16
let pack_tm_word0 h =
  let v2 = (U16.v (u8_to_u16 h.version_num) % 4) * 16384 in
  let scid = (U16.v h.spacecraft_id % 1024) * 16 in
  let vc_part = (U8.v h.virtual_channel % 64) / 4 in
  U16.uint_to_t (v2 + scid + vc_part)

val default_tm_header : vcid: vcid_t -> telemetry_header
let default_tm_header vcid = {
  version_num      = 0uy;
  spacecraft_id    = 42us;
  virtual_channel  = vcid;
  ocf_flag         = true;
  master_frame_seq = 0uy;
  virt_frame_seq   = 0uy;
  first_header_ptr = 0us;
}
