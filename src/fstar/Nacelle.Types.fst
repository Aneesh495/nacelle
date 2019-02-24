module Nacelle.Types

module U8  = FStar.UInt8
module U16 = FStar.UInt16
module U32 = FStar.UInt32
module U64 = FStar.UInt64

#set-options "--fuel 0 --ifuel 0 --z3rlimit 20"

type u8  = U8.t
type u16 = U16.t
type u32 = U32.t
type u64 = U64.t

type status_code =
  | STATUS_OK
  | STATUS_ERR_INVALID_PARAM
  | STATUS_ERR_BUFFER_OVERFLOW
  | STATUS_ERR_BUFFER_UNDERFLOW
  | STATUS_ERR_CHECKSUM_MISMATCH
  | STATUS_ERR_OUT_OF_SEQUENCE
  | STATUS_ERR_QUEUE_FULL
  | STATUS_ERR_QUEUE_EMPTY
  | STATUS_ERR_LOCKOUT
  | STATUS_ERR_TIMEOUT
  | STATUS_ERR_ANOMALY_DETECTED

let is_ok (s: status_code) : bool =
  match s with
  | STATUS_OK -> true
  | _ -> false

type byte_seq = list u8

val byte_seq_length : byte_seq -> nat
let byte_seq_length s = List.Tot.length s

type bounded_byte_seq (max_len: nat) =
  s: byte_seq{List.Tot.length s <= max_len}

type channel_id = id: u16{U16.v id < 2048}

type sequence_id = id: u16{U16.v id < 16384}

type priority_level =
  | PRIORITY_CRITICAL
  | PRIORITY_HIGH
  | PRIORITY_NORMAL
  | PRIORITY_BACKGROUND

let priority_to_u8 (p: priority_level) : u8 =
  match p with
  | PRIORITY_CRITICAL   -> 0uy
  | PRIORITY_HIGH       -> 1uy
  | PRIORITY_NORMAL     -> 2uy
  | PRIORITY_BACKGROUND -> 3uy

let u8_to_priority (v: u8) : priority_level =
  if U8.v v = 0 then PRIORITY_CRITICAL
  else if U8.v v = 1 then PRIORITY_HIGH
  else if U8.v v = 2 then PRIORITY_NORMAL
  else PRIORITY_BACKGROUND
