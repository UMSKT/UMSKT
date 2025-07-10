import re
from glob import glob
from subprocess import run, PIPE
from os.path import basename
from os import remove
from shutil import rmtree

def text_scan(text, pattern, flags=0):
    return re.findall(pattern, text, re.MULTILINE | re.IGNORECASE)

def extract(arc, fn, dirc):
    run(["7z", "e", "-y", arc, f"-o{dirc}", fn], stdout=PIPE)

def list_arc(fn):
    return run(["7z", "l", fn], stdout=PIPE).stdout.decode("utf-8")

def list_msi(fn, table):
    return run(["lessmsi", "l", "-t", table, fn], stdout=PIPE).stdout.decode("utf-8")

KNOWN_EDITIONS = [
    "enterprise",
    "pro",
    "proplus",
    "standard",
    "ultimate",
    "personal",
    "basic",
    "homestudent",
    "prohybrid",
    "excel",
    "outlook",
    "powerpoint",
    "proofing",
    "rosebud",
    "word",
    "access",
    "sharepointdesigner",
    "prjstd",
    "prjpro",
    "publisher",
    "vispro",
    "visstd"
]

if __name__ == "__main__":
    print("Filename,Edition,Installation ID Value,MPC,Is Retail")
    
    for iso in glob("isos/*.iso"):
        scanned_editions = []
        
        for edition_name in text_scan(list_arc(iso), r"\S+\.WW"):
            if edition_name in scanned_editions:
                continue
            
            msi_file = text_scan(list_arc(iso), rf"{edition_name}\\{edition_name.replace('.', '')}\.msi")[0]
            dpc_file = text_scan(list_arc(iso), rf"{edition_name}\\\S+\.DPC")[0]
            # print(edition_name, msi_file, dpc_file)
            
            extract(iso, msi_file, edition_name)
            extract(iso, dpc_file, edition_name)
            
            with open(dpc_file, "rb") as f:
                f.seek(0x98)
                inst_id_val = int.from_bytes(f.read(2), "little")
            
            mpc = text_scan(list_msi(msi_file, "Property"), "PIDTemplate,\d+")[0].split(",")[1]
            edition_tname = edition_name.split(".")[0]
            is_retail = edition_tname[-1].lower() == "r"
            
            if is_retail and edition_tname[:-1].lower() in KNOWN_EDITIONS:
                edition_tname = edition_tname[:-1]
            else:
                is_retail = False
            
            print(f"{basename(iso)},{edition_tname},{inst_id_val},{mpc},{is_retail}")
            
            rmtree(edition_name)
            
            scanned_editions.append(edition_name)