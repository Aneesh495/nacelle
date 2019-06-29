"""
Ground station protocol encoder and decoder for Nacelle flight software.
Provides verified CRC16 calculation, telecommand framing, and telemetry frame parsing.
"""

import struct

CRC16_POLY = 0x1021
CRC16_INIT = 0xFFFF

def crc16_compute(data: bytes) -> int:
    crc = CRC16_INIT
    for b in data:
        idx = ((crc >> 8) ^ b) & 0xFF
        # Table entry computation inline
        entry = idx << 8
        for _ in range(8):
            if entry & 0x8000:
                entry = ((entry << 1) ^ CRC16_POLY) & 0xFFFF
            else:
                entry = (entry << 1) & 0xFFFF
        crc = ((crc << 8) ^ entry) & 0xFFFF
    return crc

def make_space_packet(channel_id: int, seq_count: int, opcode: int, args: bytes = b"", is_cmd: bool = True) -> bytes:
    vers = 0
    k_bit = 1 if is_cmd else 0
    shf = 0
    w0 = ((vers & 0x7) << 13) | (k_bit << 12) | (shf << 11) | (channel_id & 0x7FF)
    grouping = 3  # standalone
    w1 = ((grouping & 0x3) << 14) | (seq_count & 0x3FFF)
    payload = struct.pack(">H", opcode) + args
    data_len_field = len(payload) - 1
    hdr = struct.pack(">HHH", w0, w1, data_len_field)
    return hdr + payload

def make_tc_frame(category: int, vcid: int, seq_num: int, packet_bytes: bytes) -> bytes:
    b0 = (category & 0x3) | ((vcid & 0x3F) << 2)
    b1 = seq_num & 0xFF
    body = bytes([b0, b1]) + packet_bytes
    crc = crc16_compute(body)
    return body + struct.pack(">H", crc)

def parse_tm_frame(frame_bytes: bytes) -> dict:
    if len(frame_bytes) < 1024:
        raise ValueError("Frame size too small, expected 1024 bytes")
    
    # Verify CRC
    crc_expected = struct.unpack(">H", frame_bytes[1022:1024])[0]
    crc_computed = crc16_compute(frame_bytes[:1022])
    crc_valid = (crc_expected == crc_computed)
    
    w0 = struct.unpack(">H", frame_bytes[0:2])[0]
    scid = (w0 >> 4) & 0x3FF
    vcid = (w0 >> 1) & 0x7
    has_ocf = bool(w0 & 1)
    
    m_seq = frame_bytes[2]
    v_seq = frame_bytes[3]
    fhp = struct.unpack(">H", frame_bytes[4:6])[0] & 0x7FF
    
    data_field = frame_bytes[6:1018]
    
    # OCF link report word
    ocf_word = struct.unpack(">I", frame_bytes[1018:1022])[0]
    rep_vc = (ocf_word >> 24) & 0x3F
    lockout = bool(ocf_word & (1 << 23))
    wait_flag = bool(ocf_word & (1 << 22))
    retransmit = bool(ocf_word & (1 << 21))
    report_vr = ocf_word & 0xFF
    
    # Parse telemetry housekeeping if FHP is 0
    hk_coarse = struct.unpack(">I", data_field[0:4])[0]
    hk_fine = struct.unpack(">H", data_field[4:6])[0]
    batt_mv = struct.unpack(">H", data_field[6:8])[0]
    panel_temp = struct.unpack(">H", data_field[8:10])[0]
    gyro_rate = struct.unpack(">H", data_field[10:12])[0]
    safe_hold = bool(data_field[12])
    receiver_vr = data_field[13]
    cmd_count = struct.unpack(">H", data_field[14:16])[0]
    
    return {
        "scid": scid,
        "vcid": vcid,
        "m_seq": m_seq,
        "v_seq": v_seq,
        "fhp": fhp,
        "crc_valid": crc_valid,
        "report": {
            "virtual_chan": rep_vc,
            "lockout": lockout,
            "wait_flag": wait_flag,
            "retransmit": retransmit,
            "report_val_vr": report_vr,
        },
        "housekeeping": {
            "coarse_time": hk_coarse,
            "fine_time": hk_fine,
            "battery_mv": batt_mv,
            "panel_temp_c": panel_temp / 10.0,
            "gyro_rate": gyro_rate / 10.0,
            "safe_hold": safe_hold,
            "receiver_vr": receiver_vr,
            "cmd_count": cmd_count,
        }
    }
