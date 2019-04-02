module Nacelle.Sequence.State

open Nacelle.Types
open Nacelle.Math
open Nacelle.Sequence.Types
module U8  = FStar.UInt8
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

val next_seq : s: u8 -> u8
let next_seq s =
  U8.uint_to_t ((U8.v s + 1) % 256)

val inc_u32 : c: u32 -> u32
let inc_u32 c =
  if U32.v c < 4294967295 then U32.add c 1ul else c

val process_frame : status: receiver_status -> frame: frame_header -> (receiver_status & bool)
let process_frame status frame =
  match status.state with
  | STATE_LOCKOUT ->
      if frame.category = FRAME_CONTROL_UNLOCK then
        let new_status = {
          status with
          state          = STATE_OPEN;
          retransmit_req = false;
          accepted_count = inc_u32 status.accepted_count;
        } in
        (new_status, true)
      else
        let new_status = {
          status with
          rejected_count = inc_u32 status.rejected_count;
        } in
        (new_status, false)

  | STATE_WAIT ->
      if frame.category = FRAME_CONTROL_UNLOCK then
        let new_status = {
          status with
          state          = STATE_OPEN;
          retransmit_req = false;
          accepted_count = inc_u32 status.accepted_count;
        } in
        (new_status, true)
      else if frame.category = FRAME_BYPASS then
        let new_status = {
          status with
          accepted_count = inc_u32 status.accepted_count;
        } in
        (new_status, true)
      else if frame.category = FRAME_SEQUENCED then
        if frame.frame_seq_num = status.expected_seq then
          let new_status = {
            status with
            state          = STATE_OPEN;
            expected_seq   = next_seq status.expected_seq;
            retransmit_req = false;
            accepted_count = inc_u32 status.accepted_count;
          } in
          (new_status, true)
        else
          let new_status = {
            status with
            rejected_count = inc_u32 status.rejected_count;
          } in
          (new_status, false)
      else
        (status, false)

  | STATE_OPEN ->
      if frame.category = FRAME_BYPASS then
        let new_status = {
          status with
          accepted_count = inc_u32 status.accepted_count;
        } in
        (new_status, true)
      else if frame.category = FRAME_CONTROL_UNLOCK then
        let new_status = {
          status with
          retransmit_req = false;
          accepted_count = inc_u32 status.accepted_count;
        } in
        (new_status, true)
      else if frame.category = FRAME_SEQUENCED then
        if frame.frame_seq_num = status.expected_seq then
          let new_status = {
            status with
            expected_seq   = next_seq status.expected_seq;
            accepted_count = inc_u32 status.accepted_count;
          } in
          (new_status, true)
        else if in_positive_window frame.frame_seq_num status.expected_seq status.window_width then
          let new_status = {
            status with
            state          = STATE_WAIT;
            retransmit_req = true;
            rejected_count = inc_u32 status.rejected_count;
          } in
          (new_status, false)
        else
          let new_status = {
            status with
            state          = STATE_LOCKOUT;
            rejected_count = inc_u32 status.rejected_count;
          } in
          (new_status, false)
      else
        (status, false)
