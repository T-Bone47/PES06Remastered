import bpy
import socket

print("=== Testing Blender MCP Server in Blender ===")
print("bpy.app.background:", bpy.app.background)
print("addons loaded:", list(bpy.context.preferences.addons.keys()))

server = getattr(bpy.types, "blendermcp_server", None)
print("bpy.types.blendermcp_server before:", server)

scene = bpy.context.scene
print("scene.blendermcp_port:", getattr(scene, "blendermcp_port", None))
print("scene.blendermcp_auto_start_server:", getattr(scene, "blendermcp_auto_start_server", None))
print("scene.blendermcp_server_running:", getattr(scene, "blendermcp_server_running", None))

# Try manually calling start server operator
try:
    res = bpy.ops.blendermcp.start_server()
    print("bpy.ops.blendermcp.start_server() result:", res)
except Exception as e:
    print("Exception starting server:", e)

server = getattr(bpy.types, "blendermcp_server", None)
print("bpy.types.blendermcp_server after:", server)
if server:
    print("server.running:", server.running)
    print("server.host:", server.host)
    print("server.port:", server.port)

# Check if socket is actually accepting
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.settimeout(1.0)
try:
    s.connect(("127.0.0.1", 9876))
    print("SUCCESS: Connected to 127.0.0.1:9876!")
    s.close()
except Exception as e:
    print("Socket connection failed:", e)
