module Nacelle.FDIR.Filter

open Nacelle.Types
open Nacelle.Math
open Nacelle.FDIR.Types
module U8  = FStar.UInt8
module U16 = FStar.UInt16

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

val update_channel :
  state: monitor_channel_state ->
  new_sample: u16 ->
  thresh: channel_thresholds ->
  (monitor_channel_state & recovery_action)
let update_channel state new_sample thresh =
  let sev = evaluate_severity new_sample thresh in
  if sev = SEVERITY_NOMINAL then
    let new_counter =
      if U16.v state.anomaly_counter > 0 then
        U16.sub state.anomaly_counter 1us
      else 0us
    in
    let new_state = {
      state with
      raw_value       = new_sample;
      severity        = SEVERITY_NOMINAL;
      anomaly_counter = new_counter;
    } in
    (new_state, ACTION_NONE)
  else
    let current_c = U16.v state.anomaly_counter in
    let new_c = if current_c < 65535 then current_c + 1 else current_c in
    let new_counter_u16 = U16.uint_to_t new_c in
    if new_c >= U16.v state.debounce_limit then
      let next_tier =
        if U8.v state.escalation_tier < 3 then
          U8.uint_to_t (U8.v state.escalation_tier + 1)
        else state.escalation_tier
      in
      let action =
        if U8.v next_tier = 1 then ACTION_COMPONENT_RESET
        else if U8.v next_tier = 2 then ACTION_SWITCH_SECONDARY
        else ACTION_SAFE_HOLD
      in
      let new_state = {
        state with
        raw_value       = new_sample;
        severity        = sev;
        anomaly_counter = 0us;
        escalation_tier = next_tier;
      } in
      (new_state, action)
    else
      let new_state = {
        state with
        raw_value       = new_sample;
        severity        = sev;
        anomaly_counter = new_counter_u16;
      } in
      (new_state, ACTION_NONE)

val lemma_nominal_suppresses_action :
  state: monitor_channel_state ->
  sample: u16 ->
  thresh: channel_thresholds ->
  Lemma (requires evaluate_severity sample thresh == SEVERITY_NOMINAL)
        (ensures snd (update_channel state sample thresh) == ACTION_NONE)
let lemma_nominal_suppresses_action state sample thresh = ()
