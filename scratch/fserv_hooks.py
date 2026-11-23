with open('PES 6 game files/kitserver/fserv.dll', 'rb') as f:
    dll = f.read()

import struct

# Look for 0x004xxxxx or 0x005xxxxx or 0x006xxxxx or 0x007xxxxx, which are pes6.exe code addresses
for p in range(0, len(dll)-4, 4):
    val, = struct.unpack('<I', dll[p:p+4])
    if 0x00400000 <= val <= 0x00A00000:
        # Check context
        print(f'pes6.exe address 0x{val:08X} found at dll offset 0x{p:X}')
