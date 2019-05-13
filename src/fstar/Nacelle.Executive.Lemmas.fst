module Nacelle.Executive.Lemmas

open Nacelle.Types
open Nacelle.Math
open Nacelle.Executive.Types
open Nacelle.Executive.Scheduler
module U8  = FStar.UInt8
module U32 = FStar.UInt32

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

val lemma_minor_cycle_bounded : s: executive_state ->
  Lemma (U8.v (advance_cycle s).current_minor_cycle < minor_cycles_per_major)
let lemma_minor_cycle_bounded s = ()

val lemma_dispatches_monotonic : s: executive_state ->
  Lemma (U32.v (advance_cycle s).total_dispatches >= U32.v s.total_dispatches)
let lemma_dispatches_monotonic s = ()

val lemma_empty_tasks_schedulable : minor: nat ->
  Lemma (is_minor_schedulable [] minor == true)
let lemma_empty_tasks_schedulable minor = ()
