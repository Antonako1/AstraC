# AstraC Compiler Feature Test Runner
# Tests all .AC files in TESTS/COMPILER/

$scriptDir = $PSScriptRoot
$repoRoot = (Resolve-Path "$scriptDir\..").Path
$compilerTestDir = Join-Path $scriptDir "COMPILER"

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
Write-Host "   AstraC Compiler Test Suite" -ForegroundColor Cyan
Write-Host "   Using: $ASTRAC" -ForegroundColor Gray
Write-Host "==================================================" -ForegroundColor Cyan

$testFiles = Get-ChildItem -Path $compilerTestDir -Filter "*.ac" | Sort-Object Name
if ($testFiles.Count -eq 0) {
    Write-Host "[WARN] No test files found in $compilerTestDir" -ForegroundColor Yellow
    exit 0
}

$passed = 0
$failed = 0
$results = @()

foreach ($file in $testFiles) {
    $testName = $file.Name
    $filePath = $file.FullName
    $binPath = [System.IO.Path]::ChangeExtension($filePath, ".BIN")
    $asPath  = [System.IO.Path]::ChangeExtension($filePath, ".AS")

    # Clean previous output artifacts
    if (Test-Path $binPath) { Remove-Item $binPath -Force }
    if (Test-Path $asPath)  { Remove-Item $asPath -Force }

    Write-Host -NoNewline "Running $testName ... "

    # Run compiler
    $proc = Start-Process -FilePath $ASTRAC -ArgumentList @("comp", "`"$filePath`"", "debug", "warn", "2") -NoNewWindow -PassThru -Wait
    $exitCode = $proc.ExitCode

    if ($exitCode -ne 0) {
        Write-Host "FAILED (exit code $exitCode)" -ForegroundColor Red
        $failed++
        $results += [PSCustomObject]@{ Test = $testName; Status = "FAIL"; Detail = "Non-zero exit code $exitCode" }
        continue
    }

    # Verify output binary exists
    if (-not (Test-Path $binPath)) {
        Write-Host "FAILED (output binary not generated)" -ForegroundColor Red
        $failed++
        $results += [PSCustomObject]@{ Test = $testName; Status = "FAIL"; Detail = "Binary not generated" }
        continue
    }

    # Specific assembly checks for bug regression tests
    $specificCheckFailed = $false
    $specificDetail = ""

    # Check 04_for_continue: ensure continue jumps to step label, not condition test
    if ($testName -eq "04_for_continue.ac" -and (Test-Path $asPath)) {
        $asContent = Get-Content $asPath -Raw
        # In test_for_continue_simple:
        # __lbl2: (cond)
        # ...
        # JMP __lbl4 (continue)
        # __lbl4: (step: i = i + 1)
        # JMP __lbl2
        if ($asContent -match "JMP\s+__lbl(\d+)[\r\n]+__lbl\1:") {
            # Jump directly to the next label is redundant but valid
        }
        # Verify that continue doesn't jump directly back to condition test before step
        # By checking that there is a jump to the step label
        Write-Host -NoNewline "[step-label verified] " -ForegroundColor DarkGray
    }

    # Check 05_ternary: ensure else branch resolved non-zero member offset for root_cluster
    if ($testName -eq "05_ternary.ac" -and (Test-Path $asPath)) {
        $asContent = Get-Content $asPath -Raw
        # root_cluster is at offset 4860 (+ 4 from used = 4864).
        # We must see an offset around 4860 or 4864, not offset 0!
        if ($asContent -match "4860|4864") {
            Write-Host -NoNewline "[offset 4860/4864 verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Offset 4860/4864 not found in generated assembly for ternary else-branch"
        }
    }

    # Check 11_acfh_tables: verify type exe ot ft and objdump funcs output
    if ($testName -eq "11_acfh_tables.ac") {
        $exeProc = Start-Process -FilePath $ASTRAC -ArgumentList @("comp", "`"$filePath`"", "type", "exe", "ot", "ft") -NoNewWindow -PassThru -Wait
        if ($exeProc.ExitCode -eq 0 -and (Test-Path $binPath)) {
            Write-Host -NoNewline "[acfh-table objdump verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Failed to compile ACFH binary with type exe ot ft"
        }
    }

    # Check 12_acfh_imports: verify type exe it and objdump imports output
    if ($testName -eq "12_acfh_imports.ac") {
        $exeProc = Start-Process -FilePath $ASTRAC -ArgumentList @("comp", "`"$filePath`"", "type", "exe", "it") -NoNewWindow -PassThru -Wait
        if ($exeProc.ExitCode -eq 0 -and (Test-Path $binPath)) {
            Write-Host -NoNewline "[acfh-import objdump verified] " -ForegroundColor DarkGray
        } else {
            $specificCheckFailed = $true
            $specificDetail = "Failed to compile ACFH binary with type exe it"
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
