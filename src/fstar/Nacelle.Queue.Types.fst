module Nacelle.Queue.Types

open Nacelle.Types
open Nacelle.Math
open Nacelle.Timestamp
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

noeq type queue_entry = {
  command_id    : u32;
  dispatch_time : timestamp;
  priority      : priority_level;
  channel       : channel_id;
  opcode        : u16;
  arg_len       : u8;
}

val entry_has_higher_priority : e1: queue_entry -> e2: queue_entry -> bool
let entry_has_higher_priority e1 e2 =
  let t_cmp = timestamp_compare e1.dispatch_time e2.dispatch_time in
  if t_cmp < 0 then true
  else if t_cmp > 0 then false
  else
    let p1 = U8.v (priority_to_u8 e1.priority) in
    let p2 = U8.v (priority_to_u8 e2.priority) in
    if p1 < p2 then true
    else if p1 > p2 then false
    else U32.v e1.command_id < U32.v e2.command_id

val entry_eq : e1: queue_entry -> e2: queue_entry -> bool
let entry_eq e1 e2 =
  (U32.v e1.command_id = U32.v e2.command_id) &&
  (e1.dispatch_time.coarse_secs = e2.dispatch_time.coarse_secs) &&
  (e1.dispatch_time.fine_ticks = e2.dispatch_time.fine_ticks) &&
  (priority_to_u8 e1.priority = priority_to_u8 e2.priority) &&
  (e1.channel = e2.channel) &&
  (e1.opcode = e2.opcode) &&
  (e1.arg_len = e2.arg_len)

val entry_priority_le : e1: queue_entry -> e2: queue_entry -> bool
let entry_priority_le e1 e2 =
  entry_has_higher_priority e1 e2 || entry_eq e1 e2

val lemma_higher_priority_asym : e1: queue_entry -> e2: queue_entry ->
  Lemma (requires entry_has_higher_priority e1 e2 == true)
        (ensures entry_has_higher_priority e2 e1 == false)
let lemma_higher_priority_asym e1 e2 = ()
