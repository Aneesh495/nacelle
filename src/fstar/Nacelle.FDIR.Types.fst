module Nacelle.FDIR.Types

open Nacelle.Types
open Nacelle.Math
module U16 = FStar.UInt16
module U32 = FStar.UInt32

#set-options "--fuel 0 --ifuel 0 --z3rlimit 30"

type recovery_action =
  | ACTION_NONE
  | ACTION_COMPONENT_RESET
  | ACTION_SWITCH_SECONDARY
  | ACTION_SAFE_HOLD

type channel_severity =
  | SEVERITY_NOMINAL
  | SEVERITY_WARNING
  | SEVERITY_CRITICAL

noeq type channel_thresholds = {
  warn_low  : u16;
  warn_high : u16;
  crit_low  : u16;
  crit_high : u16;
}

noeq type monitor_channel_state = {
  raw_value       : u16;
  severity        : channel_severity;
  anomaly_counter : u16;
  debounce_limit  : u16;
  escalation_tier : u8;
}

val evaluate_severity : value: u16 -> thresh: channel_thresholds -> channel_severity
let evaluate_severity value thresh =
  let v = U16.v value in
  if v < U16.v thresh.crit_low || v > U16.v thresh.crit_high then
    SEVERITY_CRITICAL
  else if v < U16.v thresh.warn_low || v > U16.v thresh.warn_high then
    SEVERITY_WARNING
  else
    SEVERITY_NOMINAL
