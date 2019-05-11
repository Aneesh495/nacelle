module Nacelle.Executive.Scheduler

open Nacelle.Types
open Nacelle.Math
open Nacelle.Executive.Types
module U8  = FStar.UInt8
module U32 = FStar.UInt32

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

val advance_cycle : s: executive_state -> executive_state
let advance_cycle s =
  let next_minor = (U8.v s.current_minor_cycle + 1) % minor_cycles_per_major in
  let next_major =
    if next_minor = 0 then
      if U32.v s.current_major_cycle < 4294967295 then
        U32.add s.current_major_cycle 1ul
      else s.current_major_cycle
    else s.current_major_cycle
  in
  let dispatches =
    if U32.v s.total_dispatches < 4294967295 then
      U32.add s.total_dispatches 1ul
    else s.total_dispatches
  in
  {
    current_major_cycle = next_major;
    current_minor_cycle = U8.uint_to_t next_minor;
    missed_deadlines    = s.missed_deadlines;
    total_dispatches    = dispatches;
  }

val sum_task_budgets_for_minor : tasks: list task_descriptor -> minor: nat -> Tot nat (decreases tasks)
let rec sum_task_budgets_for_minor tasks minor =
  match tasks with
  | [] -> 0
  | t :: rest ->
      let cost = if task_runs_in_minor t minor then t.budget_ms else 0 in
      cost + sum_task_budgets_for_minor rest minor

val is_minor_schedulable : tasks: list task_descriptor -> minor: nat -> bool
let is_minor_schedulable tasks minor =
  (sum_task_budgets_for_minor tasks minor <= minor_frame_ms)
