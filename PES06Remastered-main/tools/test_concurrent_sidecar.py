import subprocess
import time
import os
import sys
import threading

game_dir = r"C:\Users\dream\Downloads\PES06Remastered-main\test_game"
repo_dir = r"C:\Users\dream\Downloads\PES06Remastered-main\PES06Remastered-main"
sidecar_script = os.path.join(repo_dir, "sidecar", "sidecar.py")

output_holder = {}

def run_sidecar_listener():
    print("[Sidecar] Starting client listener...")
    res = subprocess.run([sys.executable, sidecar_script, "--once"],
                         cwd=repo_dir, capture_output=True, text=True, timeout=10)
    output_holder["stdout"] = res.stdout
    output_holder["stderr"] = res.stderr

t = threading.Thread(target=run_sidecar_listener)
t.start()

# Give sidecar client 0.2s to start loop
time.sleep(0.2)

print("[Game] Launching pes6.exe...")
exe_path = os.path.join(game_dir, "pes6.exe")
game_proc = subprocess.Popen([exe_path], cwd=game_dir)
print(f"[Game] pes6.exe PID {game_proc.pid} running.")

t.join(timeout=8)

time.sleep(0.5)
if game_proc.poll() is None:
    game_proc.terminate()
    try:
        game_proc.wait(timeout=2)
    except Exception:
        game_proc.kill()

print("\n--- SIDECAR CLIENT RESULT ---")
print(output_holder.get("stdout", "No stdout"))
if output_holder.get("stderr"):
    print("Stderr:", output_holder.get("stderr"))
