# AstraC Assembler Feature Test Runner
# Tests all .AS files in TESTS/ASSEMBLER/

$scriptDir = $PSScriptRoot
$repoRoot = (Resolve-Path "$scriptDir\..").Path
$asmTestDir = Join-Path $scriptDir "ASSEMBLER"

# Locate AstraC executable
$astracCandidates = @(
    (Join-Path $repoRoot "build\Release\AstraC.exe"),
    (Join-Path $repoRoot "build\AstraC.exe"),
    (Join-Path $repoRoot "AstraC.exe")
)

$ASTRAC = $null
foreach ($cand in $astracCandidates) {
    if (Test-Path $cand) {
        $ASTRAC = $cand
        break
    }
}

if (-not $ASTRAC) {
    Write-Host "[ERROR] Could not find AstraC.exe. Please build it first." -ForegroundColor Red
    exit 1
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "   AstraC Assembler Test Suite" -ForegroundColor Cyan
Write-Host "   Using: $ASTRAC" -ForegroundColor Gray
Write-Host "==================================================" -ForegroundColor Cyan

$testFiles = Get-ChildItem -Path $asmTestDir -Filter "*.AS" | Sort-Object Name
if ($testFiles.Count -eq 0) {
    Write-Host "[WARN] No test files found in $asmTestDir" -ForegroundColor Yellow
    exit 0
}

$passed = 0
$failed = 0
$results = @()

foreach ($file in $testFiles) {
    $testName = $file.Name
    $filePath = $file.FullName
    $binPath  = [System.IO.Path]::ChangeExtension($filePath, ".BIN")
    $dsmPath  = [System.IO.Path]::ChangeExtension($filePath, ".DSM")

    # Clean previous output artifacts
    if (Test-Path $binPath) { Remove-Item $binPath -Force }
    if (Test-Path $dsmPath) { Remove-Item $dsmPath -Force }

    Write-Host -NoNewline "Running $testName ... "

    $specificCheckFailed = $false
    $specificDetail = ""

    # Custom assemble args depending on test
    $asmArgs = @("asm", "`"$filePath`"")
    if ($testName -eq "07_acfh_exe_and_tables.AS") {
        $asmArgs += @("type", "exe", "ot", "ft", "it", "entry", "_start")
    }

    # Run assembler
    $proc = Start-Process -FilePath $ASTRAC -ArgumentList $asmArgs -NoNewWindow -PassThru -Wait
    $exitCode = $proc.ExitCode

    if ($exitCode -ne 0) {
        Write-Host "FAILED (exit code $exitCode)" -ForegroundColor Red
        $failed++
        $results += [PSCustomObject]@{ Test = $testName; Status = "FAIL"; Detail = "Assembler exit code $exitCode" }
        continue
    }

    if (-not (Test-Path $binPath)) {
        Write-Host "FAILED (output binary not generated)" -ForegroundColor Red
        $failed++
        $results += [PSCustomObject]@{ Test = $testName; Status = "FAIL"; Detail = "Binary not generated" }
        continue
    }

    # Run disassembler to verify instruction disassembly
    $disasmArgs = @("disasm", "`"$binPath`"")
    if ($testName -eq "06_bootloader_realmode.AS") {
        $disasmArgs += @("bits", "16", "org", "0x7C00")
    }

    $dsmProc = Start-Process -FilePath $ASTRAC -ArgumentList $disasmArgs -NoNewWindow -PassThru -Wait
    $dsmContent = if (Test-Path $dsmPath) { Get-Content $dsmPath -Raw } else { "" }

    # Test 01: Sections, data, and BSS (0 disk bytes)
    if ($testName -eq "01_data_and_sections.AS") {
        $binLen = (Get-Item $binPath).Length
        # Size should be small (around 130 bytes), not 1KB+ due to BSS
        if ($binLen -le 256) {
            Write-Host -NoNewline "[bss 0-disk-bytes ($binLen B) verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Binary too large ($binLen bytes); .bss is consuming disk bytes"
        }
    }

    # Test 02: Registers, SIB, and Control/Debug Regs (CR0-4, DR0-7)
    if ($testName -eq "02_registers_and_addressing.AS") {
        $hasCR = ($dsmContent -match "mov eax, cr0") -and ($dsmContent -match "mov cr0, eax")
        $hasDR = ($dsmContent -match "mov ebx, dr7") -and ($dsmContent -match "mov dr7, ebx")
        $hasSIB = ($dsmContent -match "\[esi\+edi\*4\+0x20\]") -or ($dsmContent -match "\[ebx\+ecx\*4\+0x10\]")
        if ($hasCR -and $hasDR -and $hasSIB) {
            Write-Host -NoNewline "[CR/DR regs & SIB verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Disassembly check failed for CR/DR/SIB (CR:$hasCR, DR:$hasDR, SIB:$hasSIB)"
        }
    }

    # Test 03: Arithmetic, Logic, Bit Instructions, Shifts/Rotates, and String/Conditionals
    if ($testName -eq "03_arithmetic_logic_bits.AS") {
        $hasShld = ($dsmContent -match "shld eax, ebx")
        $hasImul = ($dsmContent -match "imul eax, ebx, 0x0000000A")
        $hasBsf  = ($dsmContent -match "bsf eax, ebx")
        $hasSetz = ($dsmContent -match "setz al")
        $hasMovzx = ($dsmContent -match "movzx al, bl")
        if ($hasShld -and $hasImul -and $hasBsf -and $hasSetz -and $hasMovzx) {
            Write-Host -NoNewline "[shld/imul/bsf/setcc/movzx verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Disassembly check failed for instruction set (shld:$hasShld, imul:$hasImul, bsf:$hasBsf, setz:$hasSetz, movzx:$hasMovzx)"
        }
    }

    # Test 04: Control flow, jump relaxation (>127B to 0F 84), and local @@ labels
    if ($testName -eq "04_control_flow_and_labels.AS") {
        # Check that long forward jump relaxed to near form (0F 84)
        $hasRelaxedJz = ($dsmContent -match "0F 84") -or ($dsmContent -match "jz 0x000000F4")
        $hasLoop = ($dsmContent -match "loop")
        $hasJcxz = ($dsmContent -match "jcxz")
        if ($hasRelaxedJz -and $hasLoop -and $hasJcxz) {
            Write-Host -NoNewline "[jump-relaxation & loop/jcxz verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Disassembly check failed for control flow (relaxedJz:$hasRelaxedJz, loop:$hasLoop, jcxz:$hasJcxz)"
        }
    }

    # Test 05: Strings, I/O ports, system instructions
    if ($testName -eq "05_strings_ports_system.AS") {
        $hasRepMovs = ($dsmContent -match "rep movsb")
        $hasCpuid   = ($dsmContent -match "cpuid")
        $hasRdtsc   = ($dsmContent -match "rdtsc")
        $hasIo      = ($dsmContent -match "in 0x60") -or ($dsmContent -match "in al, al")
        if ($hasRepMovs -and $hasCpuid -and $hasRdtsc -and $hasIo) {
            Write-Host -NoNewline "[rep movs/cpuid/rdtsc/io verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Disassembly check failed for system/strings/IO (rep:$hasRepMovs, cpuid:$hasCpuid, rdtsc:$hasRdtsc, io:$hasIo)"
        }
    }

    # Test 06: 16-bit Real Mode MBR Boot Sector (exactly 512 bytes)
    if ($testName -eq "06_bootloader_realmode.AS") {
        $binLen = (Get-Item $binPath).Length
        $hasCli = ($dsmContent -match "00007C00\s+FA\s+cli")
        $has16Mem = ($dsmContent -match "mov ax, \[bx\+si\]")
        $hasMbrSig = $binLen -eq 512
        if ($binLen -eq 512 -and $hasCli -and $has16Mem) {
            Write-Host -NoNewline "[512-byte MBR & 16-bit realmode verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "MBR verification failed (len:$binLen (expected 512), cli:$hasCli, 16mem:$has16Mem)"
        }
    }

    # Test 07: ACFH executable with Relocations, Export Table, and Import Table
    if ($testName -eq "07_acfh_exe_and_tables.AS") {
        $objdumpOutput = & $ASTRAC objdump $binPath | Out-String
        $hasAcfhFlags = ($objdumpOutput -match "EXEC RELOCS FUNCS IMPORTS")
        $hasRelocs    = ($objdumpOutput -match "Relocation Table:") -and ($objdumpOutput -match "4 entries")
        $hasFuncs     = ($objdumpOutput -match "Function Table:") -and ($objdumpOutput -match "entries 2")
        $hasImports   = ($objdumpOutput -match "Import Table:") -and ($objdumpOutput -match "entries 4")
        $hasDsmSymbols = ($dsmContent -match "<start>:") -and ($dsmContent -match "<libc\.lib::malloc>") -and ($dsmContent -match "<libc\.lib::free>") -and ($dsmContent -match "<helper_func>")

        if ($hasAcfhFlags -and $hasRelocs -and $hasFuncs -and $hasImports -and $hasDsmSymbols) {
            Write-Host -NoNewline "[ACFH header, 4 relocs, 2 funcs, 4 imports & disasm symbols verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "ACFH verification failed (flags:$hasAcfhFlags, relocs:$hasRelocs, funcs:$hasFuncs, imports:$hasImports, dsmSym:$hasDsmSymbols)"
        }
    }

    if ($specificCheckFailed) {
        Write-Host "FAILED ($specificDetail)" -ForegroundColor Red
        $failed++
        $results += [PSCustomObject]@{ Test = $testName; Status = "FAIL"; Detail = $specificDetail }
    } else {
        Write-Host "PASSED" -ForegroundColor Green
        $passed++
        $results += [PSCustomObject]@{ Test = $testName; Status = "PASS"; Detail = "OK" }
    }
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "Test Results: $passed Passed, $failed Failed" -ForegroundColor $(if ($failed -eq 0) { "Green" } else { "Red" })
Write-Host "==================================================" -ForegroundColor Cyan

if ($failed -gt 0) {
    exit 1
}
exit 0
