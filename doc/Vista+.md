## 6. Product ID

Product ID is 5 groups of decimals, e.g.:

```
AAAAA BBB CCCCCCC DD EEE
52273 005 6861993 09 146
```

If you search `ProductID` in registry, you will find a product ID associated with your currently installed one.

The meaning of each group of numbers:

| Number    | Meaning                                                                                         |
| --------- | ----------------------------------------------------------------------------------------------- |
| `AAAAA`   | Product code. e.g.: `55661` for Windows Pro; `55660` for Windows Home.                          |
| `BBB`     | First 3 effective number digits of Primary Product Serial.                                      |
| `CCCCCCC` | Last 6 effective number digits of Primary Product Serial, and a digit of checksum.              |
| `DD`      | Public key index number for product validation. e.g.: Pro is 22, VLK is 23.                     |
| `EEE`     | Nonce (random number), for generating different install ID when doing activation via telephone. |

For `CCCCCCC` mentioned above, it had two parts including 1 digit and 6 digits. The checksum is calculated as follows: **Add all digits together (including the checksum digit), it is divisible by 7.**
For example: the last 6 digits of Primary Product Serial is `728439`, then `7 + 2 + 8 + 4 + 3 + 9 = 33` , then the checksum digit should be `2`, because `7 + 2 + 8 + 4 + 3 + 9 + 2 = 33 + 2 = 35`, and 35 is divisible by 7. So the `CCCCCCC` in Product ID should be `7284392`.

## 7. OEM related mechanism
<!-- Oh boy this section is a total mess... The original content itself is confusing as hell.. -->

For licensed product to OEMs, starting from Windows XP, Microsoft is using SLP (System-Locked Preinstallation) technology, aka.: before installing the OS, lock in the motherboard with the OS. Windows XP used SLP 1.0, it works by detecting specific string set by OEM in BIOS. This method is easy to be spoofed, but the VLK crack is more easy to use so this method is not wide-spread.
From the Windows Vista, SLP upgraded to 2.0 . Primarily the change is using cryptographic key pairs to sign the data to prevent spoofing. This requires a special SLIC (Software Licensing Internal Code) table in BIOS to support this.
In Windows 7 series of product, SLP 2.1 is used, but there is no major changes in SLP for this revision, only updated the marker in SLIC to 2.1 .

SLIC is the important part of SLP, usually directly backed in BIOS ROM, as the most underlying information of OS activation. SLIC contains the digital signature of OEM, when OS is activating, the upper level certificate must can be verified with the OEM information inside SLIC. SLIC usually contained inside SLDT(Software Licensing Description Table), which 374 bytes long, and SLDT itself is contained inside ACPI(Advanced Configuration and Power Management Interface).

OEM certificate is a certification file provided by Microsoft, including to some mandatory information provided by OEM, the extension for the file is `xrm-ms`. The OEM certificate including the OEM identification information and digitally signed by Microsoft. When OS is activating, the OEM certificate must can be validated against SLIC.

OEM product key seems no difference with retail product key, but OEM key is only valid for OEM activation, and it identifies the Windows product variation. Windows Vista cancelled the VLK model, only OEM activation is provided, and used SLP 2.0, used public key instead plain text for validation, caused difficulties for cracking. 

### Windows Vista (SLP 2.0)
<!-- Following procedure is for SLP 2.0, which is for Windows Vista. -->
The procedure for SLP 2.0 verification is as follows:

1. When installing Windows Vista, retail version user need to input the serial (CD-KEY) presented on the box. For users who got the OEM version of Windows Vista with the computer, they can find a sticker that corresponding the version they got with the computer, as a voucher for purchasing the Windows Vista OEM version. The system identifies different versions of Windows Vista based on their serial numbers, such as Home Basic, Home Advanced, Business, Ultimate, and so on. After installation, the serial number is converted into 4 sets of letters or numbers, which is the "Product ID" shown in the "System Properties". The second group is the "OEM" one, which is the serial number (CD-KEY) of the OEM version. From now on, the product ID replaces the CD-KEY <!-- as user inputted their retail CD-KEY on the box, this OEM product ID became the CD-KEY in this case -->, and the installer generates an OEM certificate for the OEM version of the installation <!-- "product certificate" maybe? And, installed to where? usually the installation is done by OEM. The information is conflicting with the "OEM certificate is a certification file provided by Microsoft" mentioned above. -->.

2. The information in the BIOS got loaded into RAM every time system boots.

3. After logging into the Windows Vista, OS calls SLP service to check the licensed rights of running product, especially the activation status. It starts to identify the licensing status of the system based on the product ID. If no product ID is detected or no valid retail version product ID is detected, it is considered as inactivated. If a valid retail version product ID is detected, it is considered successfully activated. If an OEM version of the product ID is detected, continue with the OEM verification.

4. If the OEM product ID is detected, the OEM verification process starts, and it will check the installed OEM certificate <!-- "product certificate" ? --> is valid. It mainly uses the SLIC public key from BIOS which is loaded into the RAM, to verify the digital signature of product certificate <!-- Yes, this "product certificate" came from nowhere, so I guess this the OEM certificate that installed while installing the Windows Vista by OEM. -->. If validation failed, it is considered as inactivated.

5. Verify the OEM ID field between SLIC and RSDT (Root System Description Table), and compare the OEM ID field and OEM Table ID fields between SLIC and XSDT (Extended System Description Table). If they didn't match, it is considered as inactivated.

6. After the above hurdles, the OEM license will be considered as activated. If not then fallback to inactivated and continue with normal procedure, like require user to activate the Windows.

### Windows 7 (SLP 2.1)
<!-- Following procedure is for SLP 2.1, which is for Windows 7. -->
Windows 7 still retains the OEM activation policy, but using SLP 2.1 instead.

Here is more detailed verification process:

1. After the activation process starts, if the correct SLP Key is detected, the OEM activation process starts, otherwise the WPA activation (online or telephone activation) is performed.

2. Detect the OEM certificate <!-- "product certificate" ? -->, and use the digital signature inside the OEM certificate to validate the it (Microsoft used their private key to sign the OEM certificate, and verified against the public key), if verified then continue on OEM activation process, otherwise continue on WPA activation process.

3. Compare the OEM public key, OEM ID and various fields with the information presented in the OEM certificate <!-- "product certificate" ? -->. If they match (this means OEM public key and etc. are correct), then continue on OEM activation process, otherwise continue on WPA activation process.

4. Use OEM public key verify against the digital signature of the Marker inside SLIC. if verified (this means the Message in the Marker is correct. <!-- There is no where talked about what is that Message and Marker. -->) then continue on OEM activation process, otherwise continue on WPA activation process.

5. Verify the Windows Logo <!-- I guess? "Windows Logo" part can be directly translate to "Windows Flag Mark", and this "Flag" means that actual waving thing, not a digital bit. I guess this approach is similar as the Nintendo logo bitmap inside Game Boy game cartridge ROM thing. --> inside the Marker. If the Windows Logo is present, then continue on OEM activation process, otherwise continue on WPA activation process.

6. Verify the version of Marker. If it's at least `0x20001` then continue on OEM activation process, otherwise OEM activation failed, and continue on WPA activation process.

7. Compare the OEM ID and OEM Table ID against all corresponding ACPI table headers. If they match, then OEM activation completed successfully, otherwise continue on WPA activation process.

Key points (Not containing all details, this info is only there to emphasize the key points of Microsoft encryption):

1. OEM certificate <!-- "product certificate" ? --> contains
    - Data (OEM ID and other information, Microsoft public key, etc.)
    - Hash (Hash of data)
    - Signature (Hash were signed with Microsoft private key)

2. SLIC contains:
    - Data (OEM ID, Marker version, etc., Microsoft public key, etc.)
    - Hash (Hash the data)
    - Signature (Hash were signed with Microsoft private key)

3. First, use the public key to verify whether the signatures of OEM and SLIC, then, compare the OEM ID, public key and other information in OEM and SLIC to see whether they are the same; verify the Marker version is matching the type of system currently installed.

* * *
Oh boy what a mess...

This source is from CSDN.net, and this is a known "copy paste and profit" site that infamous in China.

This article is old enough that the author is actually had integrity and cited all the actual juicy sources. However, all the actual juicy sources were long gone, because the BBS that the author cited is down，only unusable archives were left (reported by discord member @itsmefeng ). 

This article isn't providing enough information on how to crack the Windows Vista/7 activation process. We may need more actual good sources for this.

For better understanding I provided some comment (as HTML comments for Markdown compatibility). Hopefully it will be useful.

-- Some Anonymous Person
