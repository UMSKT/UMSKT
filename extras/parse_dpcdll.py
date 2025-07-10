from glob import glob
from struct import unpack
import json
import sys

def readint(f):
    return unpack("<I", f.read(4))[0]

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"usage: {sys.argv[0]} dpcdll.dll out.json")
    
    lic_types = ["NULL", "Volume", "Retail", "Evaluation", "Tablet", "OEM-SLP", "Embedded"]
    
    dpcdata = []
    
    with open(sys.argv[1], "rb") as f:
        tmp = f.read()
        f.seek(tmp.find(b"\x1e\x00\x00\x00\xff\xff\xff\x7f") - 20)
        del tmp
        
        while f.read(4) != b"\x00\x00\x00\x00":
            f.seek(-164, 1)
        
        f.seek(-4, 1)
        
        last_ind = -1
        ind = 0
        dpcentry = {}
        
        while True:
            ind = readint(f)
            bink_id = hex(readint(f))[2:].zfill(8).upper()
            min_pid = readint(f)
            max_pid = readint(f)
            
            if min_pid > 999 or max_pid > 999:
                break
            
            lic_type = readint(f)
            
            if lic_type > 6:
                break
            
            if lic_type == 2 and int(bink_id, 16) % 2 == 1:
                lic_type_str = "OEM-COA"
            else:
                lic_type_str = lic_types[lic_type]
            
            days_to_act = readint(f)
            eval_days = readint(f)
            sig_len = readint(f)
            f.read(sig_len)
            
            dpcdata.append({
                "index": ind,
                "bink": bink_id,
                "pid_range": [min_pid, max_pid],
                "type": lic_type_str,
                "days_to_activate": days_to_act,
                "days_evaluation": eval_days
            })
    
    with open(sys.argv[2], "w") as f:
        f.write(json.dumps(dpcdata, indent=4))