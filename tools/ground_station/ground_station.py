#!/usr/bin/env python3
"""
Interactive ground station console for Nacelle flight software.
Provides uplink telecommand transmission and real-time downlink telemetry reception.
"""

import os
import sys
import time
import socket
import argparse

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
from tools.ground_station.protocol import make_space_packet, make_tc_frame, parse_tm_frame

class GroundStationClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.settimeout(1.5)
        self.tc_seq_num = 0
        self.cmd_count = 0

    def send_telecommand(self, opcode: int, args: bytes = b"", category: int = 0, vcid: int = 0) -> bool:
        pkt = make_space_packet(channel_id=1, seq_count=self.cmd_count, opcode=opcode, args=args, is_cmd=True)
        frame = make_tc_frame(category=category, vcid=vcid, seq_num=self.tc_seq_num, packet_bytes=pkt)
        self.sock.sendto(frame, (self.host, self.port))
        if category == 0:  # Sequenced
            self.tc_seq_num = (self.tc_seq_num + 1) % 256
        self.cmd_count = (self.cmd_count + 1) % 16384
        return True

    def send_unlock(self) -> bool:
        # Category 2 = Unlock control frame
        frame = make_tc_frame(category=2, vcid=0, seq_num=0, packet_bytes=b"")
        self.sock.sendto(frame, (self.host, self.port))
        return True

    def recv_telemetry(self) -> dict:
        try:
            data, _ = self.sock.recvfrom(2048)
            if len(data) >= 1024:
                return parse_tm_frame(data)
        except socket.timeout:
            return None
        return None

def main():
    parser = argparse.ArgumentParser(description="Nacelle Ground Control Console")
    parser.add_argument("--host", default="127.0.0.1", help="Target node IP")
    parser.add_argument("--port", type=int, default=8088, help="Target node UDP port")
    parser.add_argument("--monitor", action="store_true", help="Monitor telemetry frames continuously")
    parser.add_argument("--cmd-reset", action="store_true", help="Send nominal reset telecommand")
    parser.add_argument("--cmd-inject-fault", action="store_true", help="Inject battery fault telecommand")
    parser.add_argument("--cmd-unlock", action="store_true", help="Send COP-1 unlock command")
    args = parser.parse_args()

    client = GroundStationClient(args.host, args.port)

    # Initial ping to establish link
    client.send_telecommand(opcode=0x1000)

    if args.cmd_reset:
        print("[GROUND] Sending nominal reset telecommand (opcode 0x1001)...")
        client.send_telecommand(opcode=0x1001)

    if args.cmd_inject_fault:
        print("[GROUND] Sending fault injection telecommand (opcode 0x2001)...")
        client.send_telecommand(opcode=0x2001)

    if args.cmd_unlock:
        print("[GROUND] Sending COP-1 unlock command...")
        client.send_unlock()

    if args.monitor:
        print(f"[GROUND] Monitoring telemetry from {args.host}:{args.port} (Ctrl+C to stop)...")
        try:
            while True:
                tm = client.recv_telemetry()
                if tm:
                    hk = tm["housekeeping"]
                    rep = tm["report"]
                    print(f"[TM] SC:{tm['scid']} VC:{tm['vcid']} M:{tm['m_seq']} V:{tm['v_seq']} | "
                          f"Time: {hk['coarse_time']}.{hk['fine_time']:04x} | "
                          f"Batt: {hk['battery_mv']}mV Temp: {hk['panel_temp_c']:.1f}C | "
                          f"SafeHold: {hk['safe_hold']} | "
                          f"LinkVR: {rep['report_val_vr']} Lockout: {rep['lockout']} Retr: {rep['retransmit']}")
                else:
                    time.sleep(0.05)
        except KeyboardInterrupt:
            print("\n[GROUND] Monitor stopped.")

if __name__ == "__main__":
    main()
