import pathlib
import re

root = pathlib.Path("c:/Users/dream/Downloads/PES06Remastered-main/PES06Remastered-main/remaster")
cpp_files = list(root.rglob("*.cpp"))

print(f"Scanning {len(cpp_files)} files...")
for p in cpp_files:
    content = p.read_text(encoding="utf-8", errors="ignore")
    lines = content.splitlines()
    for i, line in enumerate(lines, 1):
        line_s = line.strip()
        # Find global objects (outside functions)
        if re.match(r'^(static\s+)?(std::[a-zA-Z0-9_<>]+|[A-Z][a-zA-Z0-9_]+)\s+[a-zA-Z0-9_]+(\s*=\s*[^;]+)?\s*;', line_s):
            if not line_s.startswith("typedef") and not line_s.startswith("return") and "const" not in line_s:
                print(f"{p.relative_to(root)}:{i}: {line_s}")
