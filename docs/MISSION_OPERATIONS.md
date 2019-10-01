# Nacelle Mission Operations Guide

This document describes the operational interfaces, telecommand dictionary, and ground station procedures for the Nacelle flight software platform.

## Communication Interfaces

The flight node exposes a non-blocking UDP socket interface for telecommand uplinks and telemetry downlinks.

* **Default UDP Port**: `8088` (configurable via `--port <PORT>`)
* **Uplink Frame MTU**: 2048 bytes
* **Downlink Frame Size**: Exactly 1024 bytes (fixed-size transfer frames)

## Telecommand Frame Layout

Uplink telecommands are packaged inside a 2-byte frame header, followed by a variable-length space packet, terminated by a 2-byte CRC16 trailer.

```
+---------------+---------------+------------------------------+---------------+
| Byte 0        | Byte 1        | Bytes 2 .. (N-3)             | Bytes (N-2).. |
| [VCID:6|Cat:2]| [Seq Num:8]   | Space Packet Payload         | CRC16-CCITT:16|
+---------------+---------------+------------------------------+---------------+
```

* **Category (Bits 0..1)**:
  * `0`: `FRAME_SEQUENCED` (Type-A, sequence-checked against $V(R)$)
  * `1`: `FRAME_BYPASS` (Type-B, urgent expedited execution)
  * `2`: `FRAME_CONTROL_UNLOCK` (Type-C, clears lockout state)
  * `3`: `FRAME_CONTROL_SET_VR` (Type-C, overrides $V(R)$ to specified sequence number)
* **Virtual Channel ID (Bits 2..7)**: Virtual channel index (0..63).
* **Frame Sequence Number (Byte 1)**: Sequence counter modulo 256.

## Telecommand Opcode Dictionary

| Opcode | Hex    | Description | Parameters |
|--------|--------|-------------|------------|
| PING   | 0x1000 | Heartbeat test | None |
| RESET  | 0x1001 | Reset simulated subsystem state | None |
| FAULT  | 0x2001 | Inject low battery voltage condition | None |
| UNLOCK | 0x0002 | Send COP-1 unlock command | None |

## Telemetry Transfer Frame Layout

Downlink frames are strictly fixed at 1024 bytes:

```
+--------------------+--------------------------------+-------------------+----------------+
| Header (6 Bytes)   | Data Field (1012 Bytes)        | OCF Report (4 B)  | CRC16 (2 B)    |
| [SCID:10|VC:3|OCF] | Housekeeping Data + Idle Fill  | [VC:6|Flags|V(R)] | Checksum       |
+--------------------+--------------------------------+-------------------+----------------+
```

### Housekeeping Telemetry Structure (Offsets 0..15 in Data Field)

| Byte Offset | Field | Type | Description |
|-------------|-------|------|-------------|
| 0..3        | Coarse Time | uint32 | Flight elapsed seconds |
| 4..5        | Fine Time   | uint16 | Subsecond fractional ticks (0..65535) |
| 6..7        | Battery mV  | uint16 | Battery bus voltage in millivolts |
| 8..9        | Panel Temp  | uint16 | Solar panel temperature in deci-degrees C |
| 10..11      | Gyro Rate   | uint16 | Attitude rate in deci-degrees per second |
| 12          | Safe Hold   | uint8  | 1 if in autonomous safe-hold mode, else 0 |
| 13          | Receiver VR | uint8  | Current expected telecommand sequence number |
| 14..15      | Queue Count | uint16 | Commands currently pending in priority queue |

## Ground Station Usage

Start the flight node:
```bash
./build/bin/nacelle_node --port 8088
```

Launch the interactive ground station monitor:
```bash
python3 tools/ground_station/ground_station.py --port 8088 --monitor
```

Dispatch telecommands from the ground station CLI:
```bash
# Send nominal reset command
python3 tools/ground_station/ground_station.py --port 8088 --cmd-reset

# Inject fault to test autonomous FDIR
python3 tools/ground_station/ground_station.py --port 8088 --cmd-inject-fault

# Send unlock command if receiver is locked out
python3 tools/ground_station/ground_station.py --port 8088 --cmd-unlock
```

Run automated mission scenario verification:
```bash
python3 tools/ground_station/scenario_runner.py
```
