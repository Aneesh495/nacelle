module Nacelle.Executive.Types

open Nacelle.Types
open Nacelle.Math
module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

let major_frame_ms : nat = 1000
let minor_frame_ms : nat = 100
let minor_cycles_per_major : nat = 10

type task_id =
  | TASK_TELECOMMAND_INGEST
  | TASK_ATTITUDE_DETERMINATION
  | TASK_CYCLIC_FDIR_CHECK
  | TASK_PAYLOAD_POLL
  | TASK_TELEMETRY_DOWNLINK

noeq type task_descriptor = {
  id             : task_id;
  period_minors  : nat;
  budget_ms      : nat;
  enabled        : bool;
}

noeq type executive_state = {
  current_major_cycle : u32;
  current_minor_cycle : u8;
  missed_deadlines    : u32;
  total_dispatches    : u32;
}

val default_executive_state : executive_state
let default_executive_state = {
  current_major_cycle = 0ul;
  current_minor_cycle = 0uy;
  missed_deadlines    = 0ul;
  total_dispatches    = 0ul;
}

val task_runs_in_minor : t: task_descriptor -> minor: nat -> bool
let task_runs_in_minor t minor =
  t.enabled && (t.period_minors > 0) && (minor % t.period_minors = 0)
