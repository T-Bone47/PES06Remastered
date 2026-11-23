with open('PES 6 game files/kitserver/fserv.dll', 'rb') as f:
    dll = f.read()

import struct

# Function start is 0x30260
# Let's inspect bytes from 0x30260 to 0x30500
chunk = dll[0x30260 : 0x30500]
print(f'Length: {len(chunk)}')

# Let's see what WIN32 APIs are called in this range
# In PE, IAT calls look like: ff 15 <IAT_VA>
for p in range(0x30260, 0x30500):
    if dll[p:p+2] == b'\xff\x15':
        iat_va, = struct.unpack('<I', dll[p+2:p+6])
        print(f'Call IAT at 0x{p:X}: 0x{iat_va:X}')
