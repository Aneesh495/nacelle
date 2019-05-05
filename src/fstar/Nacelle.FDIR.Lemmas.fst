module Nacelle.FDIR.Lemmas

open Nacelle.Types
open Nacelle.Math
open Nacelle.FDIR.Types
open Nacelle.FDIR.Filter
module U8  = FStar.UInt8
module U16 = FStar.UInt16

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

val lemma_tier_monotonic :
  state: monitor_channel_state ->
  sample: u16 ->
  thresh: channel_thresholds ->
  Lemma (U8.v (fst (update_channel state sample thresh)).escalation_tier >= U8.v state.escalation_tier)
let lemma_tier_monotonic state sample thresh = ()

val lemma_tier_bounded :
  state: monitor_channel_state ->
  sample: u16 ->
  thresh: channel_thresholds ->
  Lemma (requires U8.v state.escalation_tier <= 3)
        (ensures U8.v (fst (update_channel state sample thresh)).escalation_tier <= 3)
let lemma_tier_bounded state sample thresh = ()

val lemma_safe_hold_absorbing :
  state: monitor_channel_state ->
  sample: u16 ->
  thresh: channel_thresholds ->
  Lemma (requires U8.v state.escalation_tier == 3)
        (ensures U8.v (fst (update_channel state sample thresh)).escalation_tier == 3)
let lemma_safe_hold_absorbing state sample thresh = ()
