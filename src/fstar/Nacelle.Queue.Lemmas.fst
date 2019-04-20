module Nacelle.Queue.Lemmas

open Nacelle.Types
open Nacelle.Math
open Nacelle.Timestamp
open Nacelle.Queue.Types
open Nacelle.Queue.Heap

#set-options "--fuel 1 --ifuel 1 --z3rlimit 30"

val lemma_insert_increases_count : h: priority_heap -> e: queue_entry ->
  Lemma (requires h.count < max_heap_capacity)
        (ensures (match heap_insert h e with
                  | Some h' -> h'.count == h.count + 1
                  | None -> False))
let lemma_insert_increases_count h e = ()

val lemma_extract_decreases_count : h: priority_heap ->
  Lemma (requires h.count > 0 /\ Cons? h.entries)
        (ensures (match heap_extract_min h with
                  | Some (_, h') -> h'.count == h.count - 1
                  | None -> False))
let lemma_extract_decreases_count h = ()

val lemma_peek_matches_extract : h: priority_heap ->
  Lemma (requires Cons? h.entries)
        (ensures (match heap_peek h, heap_extract_min h with
                  | Some e1, Some (e2, _) -> e1 == e2
                  | _ -> False))
let lemma_peek_matches_extract h = ()
