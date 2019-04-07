module Nacelle.Queue.Heap

open Nacelle.Types
open Nacelle.Math
open Nacelle.Timestamp
open Nacelle.Queue.Types
module U32 = FStar.UInt32

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

let max_heap_capacity : nat = 256

noeq type priority_heap = {
  entries : list queue_entry;
  count   : nat;
}

val empty_heap : priority_heap
let empty_heap = {
  entries = [];
  count   = 0;
}

val heap_is_empty : h: priority_heap -> bool
let heap_is_empty h = (h.count = 0)

val heap_is_full : h: priority_heap -> bool
let heap_is_full h = (h.count >= max_heap_capacity)

val insert_sorted : e: queue_entry -> l: list queue_entry -> Tot (list queue_entry) (decreases l)
let rec insert_sorted e l =
  match l with
  | [] -> [e]
  | hd :: tl ->
      if entry_has_higher_priority e hd then
        e :: hd :: tl
      else
        hd :: insert_sorted e tl

val lemma_insert_sorted_len : e: queue_entry -> l: list queue_entry ->
  Lemma (ensures List.Tot.length (insert_sorted e l) == List.Tot.length l + 1)
        (decreases l)
let rec lemma_insert_sorted_len e l =
  match l with
  | [] -> ()
  | _ :: tl ->
      if entry_has_higher_priority e (List.Tot.hd l) then ()
      else lemma_insert_sorted_len e tl

val heap_insert : h: priority_heap -> e: queue_entry -> option priority_heap
let heap_insert h e =
  if h.count >= max_heap_capacity then None
  else
    let new_entries = insert_sorted e h.entries in
    Some { entries = new_entries; count = h.count + 1 }

val heap_peek : h: priority_heap -> option queue_entry
let heap_peek h =
  match h.entries with
  | [] -> None
  | hd :: _ -> Some hd

val heap_extract_min : h: priority_heap -> option (queue_entry & priority_heap)
let heap_extract_min h =
  match h.entries with
  | [] -> None
  | hd :: tl ->
      Some (hd, { entries = tl; count = if h.count > 0 then h.count - 1 else 0 })
