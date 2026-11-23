with open('PES 6 game files/kitserver/fserv.dll', 'rb') as f:
    dll = f.read()

target = 0x2F7D4
chunk = dll[target : target + 256]
for i in range(0, len(chunk), 16):
    print(f'0x{target+i:X}: {chunk[i:i+16].hex(" ")}')
