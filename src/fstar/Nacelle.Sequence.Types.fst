module Nacelle.Sequence.Types

open Nacelle.Types
module U8  = FStar.UInt8
module U16 = FStar.UInt16

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

type receiver_state =
  | STATE_OPEN
  | STATE_WAIT
  | STATE_LOCKOUT

type frame_category =
  | FRAME_SEQUENCED
  | FRAME_BYPASS
  | FRAME_CONTROL_UNLOCK
  | FRAME_CONTROL_SET_VR

noeq type frame_header = {
  category       : frame_category;
  virtual_chan   : u8;
  frame_seq_num  : u8;
  payload_length : u16;
}

noeq type receiver_status = {
  state          : receiver_state;
  expected_seq   : u8;
  window_width   : u8;
  retransmit_req : bool;
  accepted_count : u32;
  rejected_count : u32;
}

let default_receiver_status : receiver_status = {
  state          = STATE_OPEN;
  expected_seq   = 0uy;
  window_width   = 64uy;
  retransmit_req = false;
  accepted_count = 0ul;
  rejected_count = 0ul;
}

val in_positive_window : ns: u8 -> vr: u8 -> w: u8 -> bool
let in_positive_window ns vr w =
  let diff = (U8.v ns - U8.v vr) % 256 in
  (diff < U8.v w)

val in_negative_window : ns: u8 -> vr: u8 -> w: u8 -> bool
let in_negative_window ns vr w =
  let diff = (U8.v vr - U8.v ns) % 256 in
  (diff > 0 && diff <= U8.v w)
