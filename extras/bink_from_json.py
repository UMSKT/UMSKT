from struct import pack
from json import loads
import sys

def i2b(n, b, o="little"):
    return int(n).to_bytes(b, byteorder=o)

def b2i(b, o="little"):
    return int.from_bytes(b, byteorder=o)

with open(sys.argv[1]) as f:
    kdata = loads(f.read())["BINK"][sys.argv[2]]
    
    # BINK ID, Size = 0x16c, Param Offset = 7, Checksum = 0 (for now), BINK1998 Version, Param size = 12, e bits = 28, y bits = 55
    binkdata = i2b(int(sys.argv[2], 16), 4) + i2b(364, 4) + i2b(7, 4) + i2b(0, 4) + i2b(19980206, 4) + i2b(12, 4) + i2b(28, 4) + i2b(55, 4)
    
    binkdata += i2b(kdata["p"], 48) + i2b(kdata["a"], 48) + i2b(kdata["b"], 48) + i2b(kdata["g"]["x"], 48) + i2b(kdata["g"]["y"], 48) + i2b(kdata["pub"]["x"], 48) + i2b(kdata["pub"]["y"], 48)
    
    totsum = sum([int.from_bytes(binkdata[i:i+4], byteorder="little") for i in range(4,len(binkdata),4)])
    cksum = i2b(-totsum % 4294967296, 4)
    
    binkdata = list(binkdata)
    
    for i, b in enumerate(cksum):
        binkdata[12+i] = b
    
    binkdata = bytes(binkdata)
    
    with open(sys.argv[2], "wb") as g:
        g.write(binkdata)
