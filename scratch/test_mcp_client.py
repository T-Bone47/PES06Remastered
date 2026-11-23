import socket
import json
import time

def send_command(cmd):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', 9876))
    payload = json.dumps(cmd).encode('utf-8')
    s.sendall(payload)
    
    # Read response
    chunks = []
    s.settimeout(5.0)
    try:
        while True:
            data = s.recv(8192)
            if not data:
                break
            chunks.append(data)
            # Try to decode and parse JSON to see if complete
            try:
                msg = json.loads(b''.join(chunks).decode('utf-8'))
                s.close()
                return msg
            except Exception:
                pass
    except Exception as e:
        print("Recv timeout or error:", e)
    s.close()
    return json.loads(b''.join(chunks).decode('utf-8'))

print("1. Pinging BlenderMCP Server...")
ping_resp = send_command({"type": "ping"})
print("Ping Response:", ping_resp)

print("\n2. Getting Addon Info...")
addon_resp = send_command({"type": "get_addon_info"})
print("Addon Info Status:", addon_resp.get("status"))
if addon_resp.get("status") == "success":
    res = addon_resp.get("result", {})
    print("Blender Version:", res.get("blender_version"))
    print("Addon Version:", res.get("addon_version"))

print("\n3. Getting Scene Info...")
scene_resp = send_command({"type": "get_scene_info"})
print("Scene Info Status:", scene_resp.get("status"))
if scene_resp.get("status") == "success":
    res = scene_resp.get("result", {})
    print("Scene Name:", res.get("scene_name"))
    print("Objects Count:", len(res.get("objects", [])))
    for obj in res.get("objects", [])[:10]:
        print("  -", obj.get("name"), f"({obj.get('type')})")
