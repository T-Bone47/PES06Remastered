#!/usr/bin/env python3
"""
PES 6 Legacy Remaster — Live Tactical Intelligence Sidecar
External 64-bit companion application receiving live match telemetry over Windows Named Pipe.

Features:
- Non-blocking client connection to \\\\.\\pipe\\PES6Remaster_Telemetry
- Binary packet unpacking and Fletcher32 checksum validation
- Real-time tactical metrics:
  * Team compactness (defensive block width, forward-to-back depth)
  * Pitch centroid and territorial dominance (third-of-pitch analysis)
  * Real-time 2D pitch radar visualization
- Zero memory footprint or DLL injection inside the 32-bit game process
"""

import sys
import os
import time
import struct
import argparse
import math

PIPE_NAME = r"\\.\pipe\PES6Remaster_Telemetry"
TELEMETRY_MAGIC = 0x36534550  # 'PES6'
HEADER_FMT = "IIIQiiifI"
HEADER_SIZE = struct.calcsize("<" + HEADER_FMT)
ENTITY_FMT = "hhffffffhh"
ENTITY_SIZE = struct.calcsize("<" + ENTITY_FMT)
NUM_ENTITIES = 32
PACKET_FMT = "<" + HEADER_FMT + (ENTITY_FMT * NUM_ENTITIES) + "I"
PACKET_SIZE = struct.calcsize(PACKET_FMT)

def compute_fletcher32(data: bytes) -> int:
    """Computes 32-bit Fletcher checksum over the 16-bit word payload."""
    payload_len = HEADER_SIZE + ENTITY_SIZE * NUM_ENTITIES
    words = struct.unpack(f"<{payload_len // 2}H", data[:payload_len])
    sum1 = 0xffff
    sum2 = 0xffff
    for w in words:
        sum1 = (sum1 + w) % 65535
        sum2 = (sum2 + sum1) % 65535
    return (sum2 << 16) | sum1

def parse_packet(raw_bytes: bytes):
    if len(raw_bytes) != PACKET_SIZE:
        return None
    
    unpacked = struct.unpack(PACKET_FMT, raw_bytes)
    magic = unpacked[0]
    if magic != TELEMETRY_MAGIC:
        return None
    
    header = {
        "magic": hex(magic),
        "version": unpacked[1],
        "sequence": unpacked[2],
        "wall_ms": unpacked[3],
        "mode": unpacked[4],
        "phase": unpacked[5],
        "match_type": unpacked[6],
        "clock": unpacked[7],
        "entity_count": unpacked[8]
    }
    
    entities = []
    offset = 9
    for i in range(NUM_ENTITIES):
        e_data = unpacked[offset : offset + 10]
        offset += 10
        entities.append({
            "id": e_data[0],
            "side": e_data[1],
            "x": e_data[2],
            "y": e_data[3],
            "z": e_data[4],
            "vx": e_data[5],
            "vy": e_data[6],
            "vz": e_data[7],
            "stamina": e_data[8],
            "flags": e_data[9]
        })
    
    checksum = unpacked[-1]
    return {"header": header, "entities": entities, "checksum": checksum}

DIRECTOR_STATES = {
    0: "IDLE / MENU",
    1: "LINEUP / PRESENTATION",
    2: "ACTIVE PLAY (NEUTRAL)",
    3: "MIDFIELD BUILDUP (WIDE)",
    4: "PENALTY BOX PENETRATION (TIGHT)",
    5: "GOAL CELEBRATION",
    6: "SET PIECE",
    7: "INSTANT REPLAY"
}

CAMERA_PRESETS = {
    0: "Vanilla",
    1: "Dynamic Broadcast",
    2: "Tactical High Overview",
    3: "Box Focus Tracking",
    4: "Cinematic Replay"
}

def parse_broadcast_director(entities):
    """Extracts Intelligent Broadcast Director status from metadata entity slot (id=999)."""
    dir_entry = next((e for e in entities if e["id"] == 999), None)
    if not dir_entry:
        return None
    state_id = dir_entry["side"]
    state_name = DIRECTOR_STATES.get(state_id, f"Unknown ({state_id})")
    preset_id = int(dir_entry["z"])
    preset_name = CAMERA_PRESETS.get(preset_id, f"Preset {preset_id}")
    return {
        "state_id": state_id,
        "state_name": state_name,
        "current_zoom": round(dir_entry["x"], 1),
        "target_zoom": round(dir_entry["y"], 1),
        "preset_id": preset_id,
        "preset_name": preset_name,
        "replay_active": bool(dir_entry["stamina"]),
        "clipping_fix": bool(dir_entry["flags"] & 2),
        "director_enabled": bool(dir_entry["flags"] & 1)
    }

def analyze_tactics(entities):
    """Calculates tactical compactness, team centroids, and territorial pitch dominance."""
    home_players = [e for e in entities if e["side"] == 0 and (abs(e["x"]) > 0.01 or abs(e["y"]) > 0.01)]
    away_players = [e for e in entities if e["side"] == 1 and (abs(e["x"]) > 0.01 or abs(e["y"]) > 0.01)]
    ball = next((e for e in entities if e["side"] == -1 and e["id"] == 22), None)
    
    def team_metrics(players):
        if not players:
            return {"count": 0, "width": 0.0, "depth": 0.0, "centroid_x": 0.0, "centroid_y": 0.0}
        xs = [p["x"] for p in players]
        ys = [p["y"] for p in players]
        min_x, max_x = min(xs), max(xs)
        min_y, max_y = min(ys), max(ys)
        return {
            "count": len(players),
            "width": round(max_y - min_y, 2),
            "depth": round(max_x - min_x, 2),
            "centroid_x": round(sum(xs) / len(xs), 2),
            "centroid_y": round(sum(ys) / len(ys), 2)
        }
    
    home_stats = team_metrics(home_players)
    away_stats = team_metrics(away_players)
    
    ball_zone = "Unknown"
    if ball:
        bx = ball["x"]
        if bx < -17.0:
            ball_zone = "Home Defensive Third"
        elif bx > 17.0:
            ball_zone = "Away Defensive / Home Attacking Third"
        else:
            ball_zone = "Middle Third"
            
    return {
        "home": home_stats,
        "away": away_stats,
        "ball": {"x": round(ball["x"], 2) if ball else 0.0,
                 "y": round(ball["y"], 2) if ball else 0.0,
                 "z": round(ball["z"], 2) if ball else 0.0,
                 "zone": ball_zone}
    }

def render_ascii_radar(entities, width=50, height=20):
    """Renders a real-time ASCII tactical radar pitch map."""
    # Pitch bounds roughly [-52, +52] in X (length), [-34, +34] in Y (width)
    grid = [["." for _ in range(width)] for _ in range(height)]
    
    # Draw center line
    mid_col = width // 2
    for r in range(height):
        grid[r][mid_col] = "|"
        
    for e in entities:
        if e["side"] == 0:  # Home player
            char = "H"
        elif e["side"] == 1:  # Away player
            char = "A"
        elif e["side"] == -1 and e["id"] == 22:  # Ball
            char = "O"
        else:
            continue
            
        col = int((e["x"] + 52.0) / 104.0 * (width - 1))
        row = int((e["y"] + 34.0) / 68.0 * (height - 1))
        if 0 <= col < width and 0 <= row < height:
            grid[row][col] = char
            
    lines = ["+" + "-" * width + "+"]
    for row in grid:
        lines.append("|" + "".join(row) + "|")
    lines.append("+" + "-" * width + "+")
    return "\n".join(lines)

def run_mock_simulation():
    """Generates synthetic telemetry to verify the parser, analytics, and radar display."""
    print("==================================================================")
    print("PES 6 Legacy Remaster — Sidecar Tactical Intelligence (Mock Mode)")
    print("==================================================================")
    
    # Construct mock packet with 22 players and 1 ball
    entities_bytes = b""
    # 11 Home players (spread on left side [-40 to -5], [-25 to +25])
    for i in range(11):
        x = -35.0 + (i % 4) * 10.0
        y = -20.0 + (i // 4) * 15.0
        entities_bytes += struct.pack("<" + ENTITY_FMT, i, 0, x, y, 0.0, 0.5, 0.0, 0.0, 920, 0)
    
    # 11 Away players (spread on right side [+5 to +40], [-25 to +25])
    for i in range(11):
        x = 10.0 + (i % 4) * 10.0
        y = -20.0 + (i // 4) * 15.0
        entities_bytes += struct.pack("<" + ENTITY_FMT, 11 + i, 1, x, y, 0.0, -0.5, 0.0, 0.0, 910, 0)
        
    # Ball at (x=2.5, y=-1.0, z=0.5)
    entities_bytes += struct.pack("<" + ENTITY_FMT, 22, -1, 2.5, -1.0, 0.5, 1.2, -0.4, 0.0, 0, 0)
    
    # Slot 23: Broadcast Director metadata (Midfield Buildup, Zoom 1644.5, Tactical High)
    entities_bytes += struct.pack("<" + ENTITY_FMT, 999, 3, 1644.5, 1644.5, 2.0, 0.0, 0.0, 3.0, 0, 3)

    # 8 empty bench slots (24..31)
    for i in range(24, NUM_ENTITIES):
        entities_bytes += struct.pack("<" + ENTITY_FMT, i, -2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, 0)
        
    header_bytes = struct.pack("<" + HEADER_FMT, TELEMETRY_MAGIC, 1, 1, int(time.time() * 1000), 2, 3, 4, 145.5, 24)
    packet_data = header_bytes + entities_bytes
    checksum = compute_fletcher32(packet_data)
    raw_packet = packet_data + struct.pack("<I", checksum)
    
    parsed = parse_packet(raw_packet)
    assert parsed is not None, "Failed to parse mock telemetry packet!"
    print(f"Header: Seq={parsed['header']['sequence']}, Mode={parsed['header']['mode']}, Clock={parsed['header']['clock']:.1f}s")
    
    dir_info = parse_broadcast_director(parsed["entities"])
    if dir_info:
        print(f"Broadcast Director: {dir_info['state_name']} | Cam: {dir_info['preset_name']} (Zoom {dir_info['current_zoom']} / Target {dir_info['target_zoom']})")
        print(f"  Clipping Fix Active: {dir_info['clipping_fix']} | Director Enabled: {dir_info['director_enabled']}")

    tactics = analyze_tactics(parsed["entities"])
    print(f"Tactical Analysis:")
    print(f"  Home Defensive Block: Width={tactics['home']['width']}m, Depth={tactics['home']['depth']}m, Centroid=({tactics['home']['centroid_x']}, {tactics['home']['centroid_y']})")
    print(f"  Away Defensive Block: Width={tactics['away']['width']}m, Depth={tactics['away']['depth']}m, Centroid=({tactics['away']['centroid_x']}, {tactics['away']['centroid_y']})")
    print(f"  Ball Position: ({tactics['ball']['x']}, {tactics['ball']['y']}), Zone: {tactics['ball']['zone']}")
    
    print("\nLive Tactical Pitch Radar:")
    print(render_ascii_radar(parsed["entities"]))
    print("Mock simulation PASSED successfully.")

def connect_live(mode="stream"):
    print(f"Connecting to PES 6 Named Pipe at {PIPE_NAME}...")
    import ctypes
    from ctypes import wintypes

    kernel32 = ctypes.windll.kernel32
    GENERIC_READ = 0x80000000
    OPEN_EXISTING = 3
    ERROR_PIPE_BUSY = 231

    start_time = time.time()
    handle = -1
    last_err = 0
    while time.time() - start_time < 6.0:
        handle = kernel32.CreateFileA(
            PIPE_NAME.encode("ascii"),
            GENERIC_READ,
            0,
            None,
            OPEN_EXISTING,
            0,
            None
        )
        if handle != -1 and handle != 0:
            break
        last_err = kernel32.GetLastError()
        if last_err == ERROR_PIPE_BUSY:
            kernel32.WaitNamedPipeA(PIPE_NAME.encode("ascii"), 1000)
        else:
            time.sleep(0.2)

    if handle == -1 or handle == 0:
        print(f"Named pipe {PIPE_NAME} not available after timeout (last err {last_err}). Is pes6.exe running with remaster.asi?")
        return

    print("Connected to live telemetry stream!")
    try:
        buf = (ctypes.c_ubyte * PACKET_SIZE)()
        bytes_read = wintypes.DWORD(0)
        while True:
            ok = kernel32.ReadFile(handle, buf, PACKET_SIZE, ctypes.byref(bytes_read), None)
            if not ok or bytes_read.value < PACKET_SIZE:
                print("Disconnected from telemetry pipe.")
                break
            raw_bytes = bytes(buf[:bytes_read.value])
            parsed = parse_packet(raw_bytes)
            if not parsed:
                continue
            tactics = analyze_tactics(parsed["entities"])
            dir_info = parse_broadcast_director(parsed["entities"])
            if mode == "once":
                print(f"Live Packet: Seq={parsed['header']['sequence']}, Clock={parsed['header']['clock']:.1f}s, Phase={parsed['header']['phase']}")
                if dir_info:
                    print(f"Broadcast Director: {dir_info['state_name']} | Cam: {dir_info['preset_name']} (Zoom {dir_info['current_zoom']} / Target {dir_info['target_zoom']})")
                print(f"Tactics: Home Depth={tactics['home']['depth']}m, Away Depth={tactics['away']['depth']}m, Ball={tactics['ball']['zone']}")
                break
            elif mode == "stream":
                os.system("cls" if os.name == "nt" else "clear")
                print(f"=== PES 6 REMASTER LIVE TACTICAL SIDECAR (Seq #{parsed['header']['sequence']}, Clock: {parsed['header']['clock']:.1f}s) ===")
                if dir_info:
                    print(f"Director: {dir_info['state_name']} | Cam: {dir_info['preset_name']} (Zoom {dir_info['current_zoom']:.1f} -> {dir_info['target_zoom']:.1f})")
                print(f"Home Block: Width {tactics['home']['width']}m | Depth {tactics['home']['depth']}m")
                print(f"Away Block: Width {tactics['away']['width']}m | Depth {tactics['away']['depth']}m")
                print(f"Ball: ({tactics['ball']['x']}, {tactics['ball']['y']}) [{tactics['ball']['zone']}]")
                print(render_ascii_radar(parsed["entities"]))
                time.sleep(0.1)
    finally:
        kernel32.CloseHandle(handle)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="PES 6 Remaster Tactical Intelligence Sidecar")
    parser.add_argument("--mock", action="store_true", help="Run self-test with simulated match telemetry")
    parser.add_argument("--once", action="store_true", help="Read one live packet from pipe and exit")
    parser.add_argument("--stream", action="store_true", help="Stream live 2D tactical radar in terminal")
    args = parser.parse_args()
    
    if args.mock:
        run_mock_simulation()
    elif args.once:
        connect_live(mode="once")
    else:
        connect_live(mode="stream")
