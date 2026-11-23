import subprocess
import time
import os
import sys

game_dir = r"C:\Users\dream\Downloads\PES06Remastered-main\test_game"
log_path = os.path.join(game_dir, "remaster.log")
if os.path.exists(log_path):
    try:
        os.remove(log_path)
    except Exception:
        pass

exe_path = os.path.join(game_dir, "pes6.exe")
game_proc = subprocess.Popen([exe_path], cwd=game_dir)
print(f"Launched pes6.exe with PID {game_proc.pid}")

# Wait 3.0 seconds for telemetry thread to start and create named pipe
time.sleep(3.0)

repo_dir = r"C:\Users\dream\Downloads\PES06Remastered-main\PES06Remastered-main"
sidecar_script = os.path.join(repo_dir, "sidecar", "sidecar.py")

print("Running sidecar.py --once to connect to live named pipe...")
sidecar_proc = subprocess.run([sys.executable, sidecar_script, "--once"],
                              cwd=repo_dir, capture_output=True, text=True, timeout=10)

print("--- SIDECAR OUTPUT ---")
print(sidecar_proc.stdout)
if sidecar_proc.stderr:
    print("Sidecar stderr:", sidecar_proc.stderr)

time.sleep(1)
game_proc.terminate()
try:
    game_proc.wait(timeout=2)
except Exception:
    game_proc.kill()
print("Terminated pes6.exe.")

if os.path.exists(log_path):
    print("--- REMASTER.LOG (IPC lines) ---")
    with open(log_path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if "ipc" in line or "telemetry" in line:
                print(line.strip())
