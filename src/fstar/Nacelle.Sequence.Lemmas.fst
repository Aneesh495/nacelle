module Nacelle.Sequence.Lemmas

open Nacelle.Types
open Nacelle.Math
open Nacelle.Sequence.Types
open Nacelle.Sequence.State
module U8  = FStar.UInt8
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

val lemma_accepted_sequenced_frame_exact :
  status: receiver_status ->
  frame: frame_header ->
  Lemma (requires status.state == STATE_OPEN /\
                  frame.category == FRAME_SEQUENCED /\
                  snd (process_frame status frame) == true)
        (ensures frame.frame_seq_num == status.expected_seq /\
                 (fst (process_frame status frame)).expected_seq == next_seq status.expected_seq)
let lemma_accepted_sequenced_frame_exact status frame = ()

val lemma_lockout_rejects_sequenced :
  status: receiver_status ->
  frame: frame_header ->
  Lemma (requires status.state == STATE_LOCKOUT /\
                  frame.category == FRAME_SEQUENCED)
        (ensures snd (process_frame status frame) == false /\
                 (fst (process_frame status frame)).state == STATE_LOCKOUT)
let lemma_lockout_rejects_sequenced status frame = ()

val lemma_unlock_clears_lockout :
  status: receiver_status ->
  frame: frame_header ->
  Lemma (requires status.state == STATE_LOCKOUT /\
                  frame.category == FRAME_CONTROL_UNLOCK)
        (ensures snd (process_frame status frame) == true /\
                 (fst (process_frame status frame)).state == STATE_OPEN)
let lemma_unlock_clears_lockout status frame = ()

val lemma_total_transitions :
  status: receiver_status ->
  frame: frame_header ->
  Lemma (ensures (let (s', accepted) = process_frame status frame in
                  (accepted == true \/ accepted == false)))
let lemma_total_transitions status frame = ()
