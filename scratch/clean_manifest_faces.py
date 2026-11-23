import json

paths = [
    r'PES 6 game files\remaster_pack\manifest.json',
    r'test_game\remaster_pack\manifest.json'
]

for p in paths:
    try:
        with open(p, 'r', encoding='utf-8') as f:
            data = json.load(f)
        before = len(data['assets'])
        data['assets'] = [a for a in data['assets'] if a.get('category') != 'face']
        after = len(data['assets'])
        with open(p, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
        print(f"Cleaned {p}: {before} -> {after} assets.")
    except Exception as e:
        print(f"Error {p}: {e}")
