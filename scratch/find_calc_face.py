with open('PES 6 game files/kitserver/fserv.dll', 'rb') as f:
    dll = f.read()

import re

# Find references to CalcHairFile or CalcFaceFile
pos = dll.find(b'CalcHairFile')
print('Found CalcHairFile at offset 0x%X' % pos)
str_va = 0x10000000 + 0x51000 + (pos - 0x50000)

import struct
needle = struct.pack('<I', str_va)
p = 0
while True:
    p = dll.find(needle, p)
    if p == -1: break
    print('Referenced at 0x%X' % p)
    print(dll[p-32:p+32].hex(' '))
    p += 4
