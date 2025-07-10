import hashlib

def add_pid_cksum(pid):
    sumPID = 0
    val = pid
    
    while val != 0:
        sumPID += val % 10
        val //= 10
    
    return pid * 10 + 7 - (sumPID % 7)

def decode_iid_new_version(iid, pid):
    buffer = [0] * 5
    
    for i in range(len(buffer)):
        buffer[i] = int.from_bytes(iid[i*4:i*4+4], byteorder='little')
        # print("buffer[" + str(i) + "] = " + hex(buffer[i])[2:].zfill(8))
    
    
    v1 = (buffer[3] & 0xFFFFFFF8) | 2 # Not really sure but seems to work
    v2 = (buffer[3] & 7) << 29 | buffer[2] >> 3
    hardwareId = (v1) << 32 | v2
    hardwareId = int(hardwareId).to_bytes(8, byteorder='little')
    
    
    v3 = (buffer[0] & 0xFFFFFF80) >> 7 & 0xFFFFFFFF
    unknown1 = v3 & 0x000007FF
    v4 =       v3 & 0xFFFFF800
    
    v5 = buffer[1] & 0x7F
    v6 = buffer[1] >> 7
    
    v7 = (v5 << 25 | v4) >> 11
    productID1 = v7 & 0x000003FF
    v8 =         v7 & 0xFFFFFC00
    
    v9 = (v6 >> 11) & 0x00001FFF
    v10 =        v9 & 0x00001C00
    v11 =        v9 & 0x000003FF
    
    v12 = ((v6 << 21) & 0xFFFFFFFF | v8) >> 10
    v13 = (v11 << 22) & 0xFFFFFFFF
    v14 = v13 | v12
    
    productID3RandomPart = (v14 & 0x3FF00000) >> 20
    productID2NoChecksum =  v14 & 0x000FFFFF
    
    v15 = v13 >> 30            # 0x00000003
    v16 = v10 >> 8             # 0x0000001C
    v17 = (buffer[2] & 7) << 6 # 0x000001C0
    v18 = (buffer[4] & 1) << 9 # 0x00000200
    authInfo = v18 | v17 | v16 | v15 # Not that important bug: bit 5 is not present
    
    
    productID0 = pid[0]
    productID2 = add_pid_cksum(productID2NoChecksum)
    productID3 = (pid[3] // 1000) * 1000 + productID3RandomPart
    # Just to remember: public key index I of pid (XXXXX-XXX-XXXXXXX-IIXXX) = BINK ID // 2
    
    
    # Where is actually located the version number?
    # version1 = buffer[0] & 7
    # print("Decoded IID Version1?: " + str(version1))
    
    # version2 = (int.from_bytes(iid[8:17], byteorder='little') >> 52) & 7
    # print("Decoded IID Version2?: " + str(version2))
    
    # version3 = buffer[3] & 7
    # print("Decoded IID Version3?: " + str(version3))
    
    
    if productID1 != pid[1] or productID2 != pid[2] or pid[3] % 1000 != productID3RandomPart:
        print("Error: Product ID not matching!")
        return 0, 0, 0
    
    
    return hardwareId, authInfo, unknown1

# Validate installation ID checksum
def validate_cksum(n):
    print("Checksumming installation ID...")
    n = n.replace("-", "")

    cksum = 0
    for i, k in enumerate(map(int, n)):
        if (i + 1) % 6 == 0 or i == len(n) - 1:
            print("Expected last digit", cksum % 7, "got", k)
            if cksum % 7 != k:
                return None
            
            cksum = 0
        else:
            cksum += k * (i % 2 + 1)
    
    parts = [n[i:i+5] for i in range(0, len(n), 6)]
    n_out = "".join(parts)
    
    if len(n_out) == 42:
        n_out = n_out[:-1]
    
    if len(n_out) != 45 and len(n_out) != 41:
        return None
    
    return int(n_out)

# Insert checksum digits into confirmation ID
def add_cksum(n):
    cksums = []
    n = str(n).zfill(35)
    parts = [n[i:i+5] for i in range(0, len(n), 5)]
    
    for p in parts:
        cksum = 0
        
        for i, k in enumerate(map(int, p)):
            cksum += k * (i % 2 + 1)
        
        cksums.append(str(cksum % 7))
    
    n_out = ""
    
    for i in range(7):
        n_out += parts[i] + cksums[i] + ("-" if i != 6 else "")
    
    return n_out

def encrypt(decrypted, key):
    size_half = len(decrypted) // 2
    size_half_dwords = size_half - (size_half % 4)
    last = decrypted[size_half*2:]
    decrypted = decrypted[:size_half*2]
    for i in range(4):
        first = decrypted[:size_half]
        second = decrypted[size_half:]
        # A magic byte 0x79 is now added at the beginning of the list of bytes to hash
        sha1_result = hashlib.sha1(bytearray.fromhex("79") + second + key).digest()
        sha1_result = (sha1_result[:size_half_dwords] +
                       sha1_result[size_half_dwords+4-(size_half%4) : size_half+4-(size_half%4)])
        decrypted = second + bytes(x^^y for x,y in zip(first, sha1_result))
    return decrypted + last

def decrypt(encrypted, key):
    size_half = len(encrypted) // 2
    size_half_dwords = size_half - (size_half % 4)
    last = encrypted[size_half*2:]
    encrypted = encrypted[:size_half*2]
    for i in range(4):
        first = encrypted[:size_half]
        second = encrypted[size_half:]
        # A magic byte 0x79 is now added at the beginning of the list of bytes to hash
        sha1_result = hashlib.sha1(bytearray.fromhex("79") + first + key).digest()
        sha1_result = (sha1_result[:size_half_dwords] +
                       sha1_result[size_half_dwords+4-(size_half%4) : size_half+4-(size_half%4)])
        encrypted = bytes(x^^y for x,y in zip(second, sha1_result)) + first
    return encrypted + last

# Find v of divisor (u, v) of curve y^2 = F(x)
def find_v(u):
    f = F % u
    c2 = u[1]^2 - 4 * u[0]
    c1 = 2 * f[0] - f[1] * u[1]
    
    if c2 == 0:
        if c1 == 0:
            return None
        
        try:
            v1 = sqrt(f[1]^2 / (2 * c1))
            v1.lift()
        except:
            return None
    else:
        try:
            d = 2 * sqrt(f[0]^2 + f[1] * (f[1] * u[0] - f[0] * u[1]))
            v1_1 = sqrt((c1 - d)/c2)
            v1_2 = sqrt((c1 + d)/c2)
        except:
            return None

        try:
            v1_1.lift()
            v1 = v1_1
        except:
            try:
                v1_2.lift()
                v1 = v1_2
            except:
                return None
    
    v0 = (f[1] + u[1] * v1^2) / (2 * v1)
    v = v0 + v1 * x
    
    assert (v^2 - f) % u == 0
    return v



# order of field Fp 
p = 0x16E48DD18451FE9
# Coefficients of F
coeffs = [0, 0xE5F5ECD95C8FD2, 0xFF28276F11F61, 0xFB2BD9132627E6, 0xE5F5ECD95C8FD2, 1]
# This constant inverts multiplication by 0x10001 in verification
INV = 0x01fb8cf48a70dfefe0302a1f7a5341
# Key to decrypt installation IDs
IID_KEY = b'\x5A\x30\xB9\xF3'
#"""

# minimal quadratic non-residue of p
mqnr = least_quadratic_nonresidue(p)
# Galois field of order p
Fp = GF(p)
# Polynomial field Fp[x] over Fp
Fpx.<x> = Fp[]

# Hyperellptic curve function
F = sum(k*x^i for i, k in enumerate(coeffs))
# Hyperelliptic curve E: y^2 = F(x) over Fp
E = HyperellipticCurve(F)
# The jacobian over E
J = E.jacobian()



# unpack&decrypt installationId

installationId = validate_cksum(input("Installation ID (dashes optional): "))
productId = input("Product ID (with dashes): ").split("-")
pid = [int(x) for x in productId]


# Office 2003 Professional Edition FWYTB-C7PPP-4497G-FV737-2HQWG (UMSKT generated)
# installationId = 020572391118023984229275432949036355811509788 # 020570-239116-180233-984220-927546-329495-036352-581151-097880
# pid = [73931, 746, 6952006, 57345] # 73931-746-6952006-57345


# Office 2007 Enterprise Edition XGQ68-R77XM-FPYFH-B436K-46QDY (UMSKT generated)
# installationId = 032422660398632786377841998280144793681167281 # 032424-266032-986324-786370-784193-982801-144791-368115-672814
# pid = [89388, 864, 6523093, 65443] # 89388-864-6523093-65443


print(installationId)

if not installationId:
    raise Exception("Invalid Installation ID (checksum fail)")

installationIdSize = 19 if len(str(installationId)) > 41 else 17 # 17 for XP Gold, 19 for SP1+ (includes 12 bits of sha1(product key))
iid = int(installationId).to_bytes(installationIdSize, byteorder='little')
iid = decrypt(iid, IID_KEY)
hwid, authInfo, unknown1 = decode_iid_new_version(iid, pid)

print("\nDecoded Hardware ID: " + hex(int.from_bytes(hwid, byteorder='big')))
print("Decoded AuthInfo: " + hex(authInfo))
print("Decoded Unknown1: " + hex(unknown1))

assert hwid != 0

key = hwid + int((pid[0] << 41 | pid[1] << 58 | pid[2] << 17 | pid[3]) & ((1 << 64) - 1)).to_bytes(8, byteorder='little')

data = [0x00] * 14
# data = b'\xb9g\xdd\xe1\xb0\xef-\x1e\xbd\x0frE\xd8\xbe'
print("\nConfirmation IDs:")

for i in range(0x81):
    data[6] = i # Attempt number was byte 7 in older confirmation ID version but it is now byte 6
    # Encrypt conf ID, find u of divisor (u, v)
    encrypted = encrypt(bytes(data), key)
    encrypted = int.from_bytes(encrypted, byteorder="little")
    
    x1, x2 = Fp(encrypted % p), Fp((encrypted // p) + 1)
    u1, u0 = x1 * 2, (x1 ^ 2) - ((x2 ^ 2) * mqnr)
    u = x^2 + u1 * x + u0

    # Generate original divisor
    v = find_v(u)
    
    if not v:
        print(v)
        continue
    
    d2 = J(u, v)
    divisor = d2 * INV
    
    # Get x1 and x2
    roots = [x for x, y in divisor[0].roots()]

    if len(roots) > 0:
        y = [divisor[1](r) for r in roots]
        x1 = (-roots[0]).lift()
        x2 = (-roots[1]).lift()

        if (x1 > x2) or (y[0].lift() % 2 != y[1].lift() % 2):
            x1 = (-roots[1]).lift()
            x2 = (-roots[0]).lift()
    else:
        x2 = (divisor[0][1] / 2).lift()
        x1 = sqrt((x2^2 - divisor[0][0]) / mqnr).lift() + p

    # Win
    conf_id = x1 * (p + 1) + x2
    conf_id = add_cksum(conf_id)
    print(conf_id)