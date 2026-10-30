param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$Objdump,
    [Parameter(Mandatory = $true)][ValidateSet("x86", "x64")][string]$Arch
)

$ErrorActionPreference = "Stop"

function Fail([string]$Message) {
    Write-Host "Windows baseline check failed: $Message"
    exit 1
}

if (-not (Test-Path -LiteralPath $Exe)) {
    Fail "missing binary: $Exe"
}
if (-not (Test-Path -LiteralPath $Objdump)) {
    Fail "missing objdump: $Objdump"
}

$format = & $Objdump -f -- $Exe 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) {
    Fail "objdump -f failed: $format"
}

$expectFormat = $(if ($Arch -eq "x86") { "pei-i386" } else { "pei-x86-64" })
if ($format -notmatch [regex]::Escape($expectFormat)) {
    Fail "expected a $expectFormat binary"
}

$pe = & $Objdump -p -- $Exe 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) {
    Fail "objdump -p failed: $pe"
}

function PeField([string]$Text, [string]$Name) {
    if ($Text -match ("(?m)^\s*" + [regex]::Escape($Name) + "\s+(\d+)")) {
        return [int]$Matches[1]
    }
    return $null
}

$osMajor = PeField $pe "MajorOSystemVersion"
$osMinor = PeField $pe "MinorOSystemVersion"
$subMajor = PeField $pe "MajorSubsystemVersion"
$subMinor = PeField $pe "MinorSubsystemVersion"
if ($Arch -eq "x86") {
    if ($osMajor -ne 4 -or $osMinor -ne 0 -or $subMajor -ne 4 -or $subMinor -ne 0) {
        Fail "expected Windows 98 / NT 4 subsystem 4.0, got OS $osMajor.$osMinor subsystem $subMajor.$subMinor"
    }
} else {
    if ($osMajor -ne 5 -or $osMinor -ne 2 -or $subMajor -ne 5 -or $subMinor -ne 2) {
        Fail "expected XP x64 subsystem 5.2, got OS $osMajor.$osMinor subsystem $subMajor.$subMinor"
    }
}

$allowedDlls = @{
    "kernel32.dll" = $true
}
$badImports = @{
    "encodepointer" = $true
    "decodepointer" = $true
    "encodesystempointer" = $true
    "decodesystempointer" = $true
    "gettickcount64" = $true
    "initializecriticalsectionex" = $true
    "flsalloc" = $true
    "flsfree" = $true
    "flsgetvalue" = $true
    "flssetvalue" = $true
    "comparestringex" = $true
    "lcmapstringex" = $true
    "getlocaleinfoex" = $true
    "lcidtolocalename" = $true
    "localenametolcid" = $true
    "initializeconditionvariable" = $true
    "sleepconditionvariablecs" = $true
    "sleepconditionvariablesrw" = $true
    "wakeconditionvariable" = $true
    "wakeallconditionvariable" = $true
    "initializesrwlock" = $true
    "acquiresrwlockexclusive" = $true
    "releasesrwlockexclusive" = $true
    "tryacquiresrwlockexclusive" = $true
    "getfileinformationbyhandleex" = $true
    "setfileinformationbyhandle" = $true
    "getfinalpathnamebyhandlea" = $true
    "getfinalpathnamebyhandlew" = $true
    "createsymboliclinka" = $true
    "createsymboliclinkw" = $true
    "getmodulehandleexa" = $true
    "getmodulehandleexw" = $true
    "iswow64process" = $true
    "interlockedcompareexchange64" = $true
    "heapsetinformation" = $true
    "__getmainargs" = $true
    "__wgetmainargs" = $true
    "__set_app_type" = $true
    "_amsg_exit" = $true
    "__p__fmode" = $true
    "__p__commode" = $true
    "__p__environ" = $true
    "getnativesysteminfo" = $true
    "createtoolhelp32snapshot" = $true
    "process32first" = $true
    "process32firsta" = $true
    "process32firstw" = $true
    "process32next" = $true
    "process32nexta" = $true
    "process32nextw" = $true
    "loadlibraryw" = $true
}

$importHits = New-Object System.Collections.Generic.List[string]
foreach ($line in ($pe -split "`r?`n")) {
    if ($line -match "DLL Name:\s*(\S+)") {
        $dll = $Matches[1]
        if (-not $allowedDlls.ContainsKey($dll.ToLowerInvariant())) {
            $importHits.Add("DLL $dll")
        }
    }
    if ($line -match "^\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+(\S+)") {
        $name = $Matches[1]
        if ($name -match "^(.*)@\d+$") {
            $name = $Matches[1]
        }
        if ($badImports.ContainsKey($name.ToLowerInvariant())) {
            $importHits.Add($name)
        }
    }
}
if ($importHits.Count -gt 0) {
    $shown = ($importHits | Select-Object -First 20) -join ", "
    Fail "imports outside kernel32, or APIs missing on Windows 98 / NT 4: $shown"
}

if ($Arch -eq "x86") {
    $badMnemonic = "(?i)^(cmov[a-z]*|fcmov[a-z]*|emms|nopl|nopw|fcomi|fcomip|fucomi|fucomip|movaps|movups|movapd|movupd|movdqa|movdqu|movss|movhps|movlps|movlhps|movhlps|xorps|xorpd|andps|andpd|orps|orpd|addps|addpd|addss|addsd|subps|subpd|subss|subsd|mulps|mulpd|mulss|mulsd|divps|divpd|divss|divsd|cvtsi2ss|cvtsi2sd|cvttss2si|cvttsd2si|cvtss2si|cvtsd2si|pshufd|pshuflw|pshufhw|pshufw|punpck[a-z]+|packss[a-z]+|packus[a-z]+|pcmp[a-z]+|padd[bwdq]|psub[bwdq]|pmullw|pmulhw|pmulhuw|pmaddwd|pavgb|pavgw|pextrw|pinsrw|pmovmskb|prefetch[a-z]*|sfence|mfence|lfence|maskmovq|movntq|movntps|lddqu|pause)\b"
    $badRegister = "(?i)%(mm|xmm|ymm|zmm)[0-9]"
    $insnHits = New-Object System.Collections.Generic.List[string]
    $disasm = & $Objdump -d -M att -- $Exe 2>&1
    if ($LASTEXITCODE -ne 0) {
        Fail "objdump -d failed"
    }
    foreach ($line in $disasm) {
        $text = [string]$line
        $insn = $null
        if ($text.Contains("`t")) {
            $parts = $text.Split("`t")
            $insn = $parts[$parts.Length - 1].Trim()
        }
        if (-not $insn) {
            continue
        }
        if ($insn -match $badMnemonic -or $insn -match $badRegister) {
            $insnHits.Add($text.Trim())
            if ($insnHits.Count -ge 20) {
                break
            }
        }
    }
    if ($insnHits.Count -gt 0) {
        Fail ("forbidden instructions:`n" + ($insnHits -join "`n"))
    }
}

Write-Host "Windows baseline check passed ($Arch, kernel32 only): $Exe"
exit 0
