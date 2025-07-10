from struct import pack, unpack, calcsize
from json import dumps
from os.path import basename
import sys

def readint(f):
    return unpack("<I", f.read(4))[0]

def readstc(f, s):
    s = "<" + s
    sz = calcsize(s)
    return unpack(s, f.read(sz))

pubkey_data = {}

with open(sys.argv[1], "rb") as f:
    magic1 = readint(f)
    
    if magic1 != 0x44556677:
        raise Exception("Invalid pubkey format")
    
    f.read(4)
    
    field_data_size = readint(f)
    magic2 = readint(f)
    
    if magic2 != 0x00112233:
        raise Exception("Invalid pubkey format")
    
    f.read(4)
    
    data = readstc(f, "B" * 3 + "I" * 9)
    
    must_be_0 = data[0]
    
    if must_be_0 != 0:
        raise Exception("Invalid field data")
    
    size_modulus = data[1]
    size_order = data[2]
    ext_deg1 = data[3]
    ext_deg2 = data[4]
    offset_modulus = data[8]
    
    f.read(1)
    h1_bases = list(readstc(f, "B" * size_modulus))
    
    modulus = int.from_bytes(f.read(size_modulus), "little")
    order = int.from_bytes(f.read(size_order), "little")
    ext_minpoly1 = list(readstc(f, "B" * (ext_deg1 + 1)))
    ext_minpoly2 = list(readstc(f, "B" * (ext_deg2 + 1)))
    
    f.read(size_modulus * 2)
    
    ec_a = int.from_bytes(f.read(size_modulus), "little")
    ec_b = int.from_bytes(f.read(size_modulus), "little")
    
    f.seek(field_data_size + 12)
    
    points = []
    for i in range(size_modulus):
        x = []
        y = []
        
        for i in range(ext_deg1):
            x.append(int.from_bytes(f.read(size_modulus), "little"))
        
        for i in range(ext_deg1):
            y.append(int.from_bytes(f.read(size_modulus), "little"))
        
        points.append({"x": x, "y": y})
    
    pairing_val = []
    for i in range(ext_deg2):
        ext1_val = []
        for j in range(ext_deg1):
            ext1_val.append(int.from_bytes(f.read(size_modulus), "little"))
        
        pairing_val.append(ext1_val)
    
    pubkey_data = {
        "field": {
            "modulus": modulus,
            "ec_base_order": order,
            "k3_minpoly": ext_minpoly1,
            "k6_minpoly": ext_minpoly2,
        },
        "h1_bases": h1_bases,
        "curve": {
            "a": ec_a,
            "b": ec_b
        },
        "points": points,
        "pairing_val": pairing_val
    }
    
    with open("pubkey_info/" + basename(sys.argv[1]).replace(".pubkey", ".json"), "w") as g:
        g.write(dumps(pubkey_data, indent=4))