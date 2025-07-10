from Crypto.Cipher import ARC4
from hashlib import sha1, md5
from random import randint
from ecutils.core import Point, EllipticCurve
from sys import argv

KCHARS = "BCDFGHJKMPQRTVWXY2346789"

SPK_ECKEY = {
    "a": 1,
    "b": 0,
    "g": {
        "x": 10692194187797070010417373067833672857716423048889432566885309624149667762706899929433420143814127803064297378514651,
        "y": 14587399915883137990539191966406864676102477026583239850923355829082059124877792299572208431243410905713755917185109
    },
    "n": 629063109922370885449,
    "p": 21782971228112002125810473336838725345308036616026120243639513697227789232461459408261967852943809534324870610618161,
    "priv": 153862071918555979944,
    "pub": {
        "x": 3917395608307488535457389605368226854270150445881753750395461980792533894109091921400661704941484971683063487980768,
        "y": 8858262671783403684463979458475735219807686373661776500155868309933327116988404547349319879900761946444470688332645
    }
}

LKP_ECKEY = {
    "a": 1,
    "b": 0,
    "g": {
        "x": 18999816458520350299014628291870504329073391058325678653840191278128672378485029664052827205905352913351648904170809,
        "y": 7233699725243644729688547165924232430035643592445942846958231777803539836627943189850381859836033366776176689124317
    },
    "n": 675048016158598417213,
    "p": 28688293616765795404141427476803815352899912533728694325464374376776313457785622361119232589082131818578591461837297,
    "priv": 100266970209474387075,
    "pub": {
        "x": 7147768390112741602848314103078506234267895391544114241891627778383312460777957307647946308927283757886117119137500,
        "y": 20525272195909974311677173484301099561025532568381820845650748498800315498040161314197178524020516408371544778243934
    }
}


def encode_pkey(n):
    out = ""
    
    while n > 0:
        out = KCHARS[n % 24] + out
        n //= 24
    
    out = "-".join([out[i:i+5] for i in range(0, len(out), 5)])
    return out

def decode_pkey(k):
    k = k.replace("-", "")
    out = 0
    
    for c in k:
        out *= 24
        out += KCHARS.index(c)
    
    return out

def int_to_bytes(n, l=None):
    n = int(n)
    
    if not l:
        l = (n.bit_length() + 7) // 8
    
    return n.to_bytes(l, byteorder="little")

def make_curve(curve_def):
    G = Point(x=curve_def["g"]["x"], y=curve_def["g"]["y"])
    K = Point(x=curve_def["pub"]["x"], y=curve_def["pub"]["y"])
    E = EllipticCurve(p=curve_def["p"], a=curve_def["a"], b=curve_def["b"], G=G, n=curve_def["n"], h=1)
    
    return E, G, K

def get_spkid(pid):
    spkid_s = pid[10:16] + pid[18:23]
    return int(spkid_s.split("-")[0])

def validate_tskey(pid, tskey, is_spk=True):
    keydata = decode_pkey(tskey).to_bytes(21, "little")
    rk = md5(pid.encode("utf-16-le")).digest()[:5] + b"\x00" * 11
    c = ARC4.new(rk)
    dc_kdata = c.decrypt(keydata)
    keydata = dc_kdata[:7]
    
    sigdata = int.from_bytes(dc_kdata[7:], "little")
    h = sigdata & 0x7ffffffff
    s = (sigdata >> 35) & 0x1fffffffffffffffff

    params = SPK_ECKEY if is_spk else LKP_ECKEY
    E, G, K = make_curve(params)
    R = E.add_points(E.multiply_point(h, K), E.multiply_point(s, G))
    md = sha1(keydata + int_to_bytes(R.x, 48) + int_to_bytes(R.y, 48)).digest()
    ht = ((int.from_bytes(md[4:8], "little") >> 29) << 32) | (int.from_bytes(md[:4], "little"))
    
    spkid = int.from_bytes(keydata, "little") & 0x1FFFFFFFFF
    
    return h == ht and (not is_spk or spkid == get_spkid(pid))

def generate_tskey(pid, keydata, is_spk=True):
    params = SPK_ECKEY if is_spk else LKP_ECKEY
    priv = SPK_ECKEY["priv"] if is_spk else LKP_ECKEY["priv"]
    
    E, G, K = make_curve(params)
    s = 0
    
    while True:
        c = randint(1, E.n - 1)
        R = E.multiply_point(c, G)
         
        md = sha1(keydata + int_to_bytes(R.x, 48) + int_to_bytes(R.y, 48)).digest()
        h = ((int.from_bytes(md[4:8], "little") >> 29) << 32) | (int.from_bytes(md[:4], "little")) 
        s = ((-priv * h + c) % E.n) & 0x1fffffffffffffffff
        
        keyinf = int.from_bytes(keydata, "little")
        pkdata = ((s << 91) | (h << 56) | keyinf).to_bytes(21, "little")
        rk = md5(pid.encode("utf-16-le")).digest()[:5] + b"\x00" * 11
        c = ARC4.new(rk)
        pke = c.encrypt(pkdata)[:20]
        pk = int.from_bytes(pke, "little")
        pkstr = encode_pkey(pk)
        
        if s < 0x1fffffffffffffff and validate_tskey(pid, pkstr, is_spk):
            return pkstr

def generate_spk(pid):
    spkid = get_spkid(pid)
    spkdata = spkid.to_bytes(7, "little")
    
    return generate_tskey(pid, spkdata)

def generate_lkp(pid, count, major_ver, minor_ver, chid):
    version = 1
    
    if (major_ver == 5 and minor_ver > 0) or major_ver > 5:
        version = (major_ver << 3) | minor_ver
    
    lkpinfo = (chid << 46) | (count << 32) | (2 << 18) | (144 << 10) | (version << 3)
    
    lkpdata = lkpinfo.to_bytes(7, "little")
    
    return generate_tskey(pid, lkpdata, False)

if __name__ == "__main__":
    if len(argv) == 2:
        pid = argv[1]
        print(f"License Server ID: {generate_spk(pid)}")
    elif len(argv) == 5:
        pid = argv[1]
        count = int(argv[2])
        ver_major, ver_minor = map(int, argv[3].split("."))
        chid = int(argv[4])
        
        print(f"License Key Pack ID: {generate_lkp(pid, count, ver_major, ver_minor, chid)}")
    else:
        print(f"Usage: {argv[0]} <pid> [<count> <version> <chid>]")
        print(f"Example: {argv[0]} 00490-92005-99454-AT527 1234 10.3 32")