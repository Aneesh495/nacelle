module Nacelle.Sequence.Report

open Nacelle.Types
open Nacelle.Math
open Nacelle.Sequence.Types
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 20"

noeq type link_report = {
  virtual_chan   : u8;
  lockout        : bool;
  wait_flag      : bool;
  retransmit     : bool;
  report_val_vr  : u8;
}

val generate_report : status: receiver_status -> vcid: u8 -> link_report
let generate_report status vcid =
  {
    virtual_chan   = U8.uint_to_t (U8.v vcid % 64);
    lockout        = (status.state = STATE_LOCKOUT);
    wait_flag      = (status.state = STATE_WAIT);
    retransmit     = status.retransmit_req;
    report_val_vr  = status.expected_seq;
  }

val pack_report_word : r: link_report -> u32
let pack_report_word r =
  let vc_part  = U32.shift_left (u8_to_u32 r.virtual_chan) 24ul in
  let lock_bit = if r.lockout then 0x00800000ul else 0ul in
  let wait_bit = if r.wait_flag then 0x00400000ul else 0ul in
  let retr_bit = if r.retransmit then 0x00200000ul else 0ul in
  let vr_part  = u8_to_u32 r.report_val_vr in
  U32.logor (U32.logor vc_part lock_bit) (U32.logor (U32.logor wait_bit retr_bit) vr_part)

val unpack_report_word : w: u32 -> link_report
let unpack_report_word w =
  let vc = U8.uint_to_t (U32.v (U32.shift_right w 24ul) % 64) in
  let lock = (U32.v (U32.logand (U32.shift_right w 23ul) 1ul) = 1) in
  let wait = (U32.v (U32.logand (U32.shift_right w 22ul) 1ul) = 1) in
  let retr = (U32.v (U32.logand (U32.shift_right w 21ul) 1ul) = 1) in
  let vr = U8.uint_to_t (U32.v w % 256) in
  {
    virtual_chan   = vc;
    lockout        = lock;
    wait_flag      = wait;
    retransmit     = retr;
    report_val_vr  = vr;
  }

val lemma_generate_report_vr : status: receiver_status -> vcid: u8 ->
  Lemma ((generate_report status vcid).report_val_vr == status.expected_seq)
let lemma_generate_report_vr status vcid = ()
