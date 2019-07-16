#!/usr/bin/env python3
"""
Automated end-to-end mission scenario test runner for Nacelle flight software.
Launches the flight node, executes mission scenarios, verifies invariants, and terminates cleanly.
"""

import os
import sys
import time
import subprocess
import socket

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from tools.ground_station.ground_station import GroundStationClient
from tools.ground_station.protocol import make_space_packet, make_tc_frame

def run_scenarios(binary_path: str, port: int = 9933):
    print("==========================================================")
    print(" Nacelle End-to-End Flight Mission Scenarios")
    print("==========================================================")

    proc = subprocess.Popen([binary_path, "--port", str(port)])
    time.sleep(0.3)

    client = GroundStationClient("127.0.0.1", port)
    all_passed = True

    try:
        # ----------------------------------------------------
        # Scenario 1: Nominal Uplink & Sequence Progression
        # ----------------------------------------------------
        print("[SCENARIO 1] Nominal Uplink & Sequence Progression ...")
        # Send initial command to register client address with flight node
        client.send_telecommand(opcode=0x1000)
        time.sleep(0.1)

        for _ in range(5):
            client.send_telecommand(opcode=0x1001)
            time.sleep(0.05)

        # Receive telemetry until expected sequence reflects all 6 accepted commands
        vr = 0
        for _ in range(10):
            tm = client.recv_telemetry()
            if tm:
                assert tm["crc_valid"], "Telemetry frame CRC invalid"
                vr = tm["report"]["report_val_vr"]
                if vr >= 6:
                    break
            time.sleep(0.1)

        print(f"  --> Flight Receiver V(R) = {vr} (Expected 6)")
        assert vr == 6, f"Expected V(R) == 6, got {vr}"
        print("  [PASS] Scenario 1 completed successfully.")

        # ----------------------------------------------------
        # Scenario 2: Frame Corruption & Recovery
        # ----------------------------------------------------
        print("[SCENARIO 2] Out-of-Order Sequence & Retransmit Recovery ...")
        # Corrupt sequence: jump ahead by 5
        corrupted_seq = (client.tc_seq_num + 5) % 256
        corrupt_pkt = make_space_packet(channel_id=1, seq_count=99, opcode=0x1000)
        corrupt_frame = make_tc_frame(category=0, vcid=0, seq_num=corrupted_seq, packet_bytes=corrupt_pkt)
        client.sock.sendto(corrupt_frame, (client.host, client.port))

        # Receive telemetry, verify wait or retransmit flag
        wait_observed = False
        for _ in range(8):
            tm_err = client.recv_telemetry()
            if tm_err:
                rep = tm_err["report"]
                if rep["wait_flag"] or rep["retransmit"]:
                    wait_observed = True
                    print(f"  --> Receiver State: Wait={rep['wait_flag']}, Retransmit={rep['retransmit']}")
                    break
            time.sleep(0.1)

        assert wait_observed, "Expected WAIT state on out-of-order frame"

        # Now send the actual expected sequence frame
        client.send_telecommand(opcode=0x1001)

        recovered = False
        for _ in range(8):
            tm_rec = client.recv_telemetry()
            if tm_rec:
                rep = tm_rec["report"]
                if rep["report_val_vr"] == 7 and not rep["wait_flag"]:
                    recovered = True
                    print(f"  --> Recovered V(R) = {rep['report_val_vr']}, Wait = {rep['wait_flag']}")
                    break
            time.sleep(0.1)

        assert recovered, "Expected recovery to OPEN state after retransmitting expected frame"
        print("  [PASS] Scenario 2 completed successfully.")

        # ----------------------------------------------------
        # Scenario 3: Autonomous FDIR Anomaly & Safe Hold
        # ----------------------------------------------------
        print("[SCENARIO 3] Autonomous FDIR Anomaly Detection & Safe-Hold Escalation ...")
        # Inject battery undervoltage anomaly via command opcode 0x2001
        client.send_telecommand(opcode=0x2001)

        safe_hold_observed = False
        for _ in range(15):
            tm_fdir = client.recv_telemetry()
            if tm_fdir:
                safe_hold = tm_fdir["housekeeping"]["safe_hold"]
                batt_mv = tm_fdir["housekeeping"]["battery_mv"]
                if safe_hold:
                    safe_hold_observed = True
                    print(f"  --> Telemetry Battery = {batt_mv} mV, Safe-Hold Active = {safe_hold}")
                    break
            time.sleep(0.1)

        assert safe_hold_observed, "Expected spacecraft to enter Safe-Hold mode"
        print("  [PASS] Scenario 3 completed successfully.")

    except Exception as e:
        print(f"  [FAIL] Scenario failed with error: {e}")
        all_passed = False
    finally:
        proc.terminate()
        proc.wait()

    print("==========================================================")
    if all_passed:
        print(" ALL FLIGHT MISSION SCENARIOS PASSED WITH ZERO ERRORS")
    else:
        print(" SOME SCENARIOS FAILED")
    print("==========================================================")
    return 0 if all_passed else 1

if __name__ == "__main__":
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    bin_path = os.path.join(root_dir, "build", "bin", "nacelle_node")
    sys.exit(run_scenarios(bin_path, port=9933))
