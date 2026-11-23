import subprocess
import time
import os

game_dir = r"C:\Users\dream\Downloads\PES06Remastered-main\test_game"
log_path = os.path.join(game_dir, "remaster.log")
if os.path.exists(log_path):
    try:
        os.remove(log_path)
    except Exception as e:
        print(f"Warning removing old log: {e}")

exe_path = os.path.join(game_dir, "pes6.exe")
p = subprocess.Popen([exe_path], cwd=game_dir)
print(f"Launched pes6.exe with PID {p.pid}")
time.sleep(5)
p.terminate()
try:
    p.wait(timeout=2)
except Exception:
    p.kill()
print("Terminated test process.")

if os.path.exists(log_path):
    print("--- REMASTER.LOG OUTPUT ---")
    with open(log_path, "r", encoding="utf-8", errors="replace") as f:
        print(f.read())
else:
    print("remaster.log not found!")
