#!/usr/bin/env bash
# Formally verified module checker for Nacelle
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FSTAR="${FSTAR:-/Users/aneeshkrishna/.local/fstar/bin/fstar.exe}"

echo "=========================================================="
echo " Nacelle F* Formal Verification Suite"
echo "=========================================================="

MODULES=(
  "Nacelle.Types.fst"
  "Nacelle.Math.fst"
  "Nacelle.Checksum.BitStep.fst"
  "Nacelle.Checksum.Table.fst"
  "Nacelle.Timestamp.fst"
  "Nacelle.Packet.Header.fst"
  "Nacelle.Packet.Parser.fst"
  "Nacelle.Packet.Serializer.fst"
  "Nacelle.Packet.Lemmas.fst"
  "Nacelle.Sequence.Types.fst"
  "Nacelle.Sequence.State.fst"
  "Nacelle.Sequence.Lemmas.fst"
  "Nacelle.Sequence.Report.fst"
  "Nacelle.Telemetry.Types.fst"
  "Nacelle.Telemetry.Multiplexer.fst"
  "Nacelle.Telemetry.Idle.fst"
  "Nacelle.Queue.Types.fst"
  "Nacelle.Queue.Heap.fst"
  "Nacelle.Queue.Lemmas.fst"
  "Nacelle.FDIR.Types.fst"
  "Nacelle.FDIR.Filter.fst"
  "Nacelle.FDIR.Lemmas.fst"
  "Nacelle.Executive.Types.fst"
  "Nacelle.Executive.Scheduler.fst"
  "Nacelle.Executive.Lemmas.fst"
)

TOTAL=${#MODULES[@]}
PASSED=0

cd "$PROJECT_ROOT"
mkdir -p build/fstar_cache

for MOD in "${MODULES[@]}"; do
  echo -n "[VERIFY] src/fstar/$MOD ... "
  "$FSTAR" --include src/fstar --odir build/fstar_cache --cache_checked_modules "src/fstar/$MOD" > /dev/null 2>&1
  echo "PASSED"
  PASSED=$((PASSED + 1))
done

echo "=========================================================="
echo " Verification Result: $PASSED / $TOTAL modules verified successfully"
echo " Zero errors. All inductive invariants and theorems proved."
echo "=========================================================="
