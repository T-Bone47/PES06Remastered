with open('PES 6 game files/kitserver/fserv.dll', 'rb') as f:
    dll = f.read()

import struct

# Let's inspect strings around 0x507EC
print('Strings in .rdata around 0x50700 to 0x50900:')
sdata = dll[0x50700 : 0x50900]
for p in range(0, len(sdata)):
    if sdata[p:p+1] >= b' ' and sdata[p:p+1] <= b'~':
        end = p
        while end < len(sdata) and sdata[end:end+1] >= b' ' and sdata[end:end+1] <= b'~':
            end += 1
        if end - p > 4:
            print(f'  0x{0x50700+p:X}: {sdata[p:end].decode("latin1")}')
            p = end
