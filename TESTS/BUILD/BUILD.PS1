# Build multi-stage bootloader + kernel floppy image
# Requires: AstraC.exe in build\Release\

$scriptDir = $PSScriptRoot
Push-Location $scriptDir

# Paths
$ASTRAC = Join-Path $scriptDir "..\..\build\Release\AstraC.exe" -resolve
$bootAsm  = Join-Path $scriptDir "..\BOOTLOADER\BOOTLOADER.AS" -resolve
$stageAsm = Join-Path $scriptDir "..\BOOTLOADER\SECOND_STAGE.AS" -resolve
$kernelAc = Join-Path $scriptDir "..\KERNEL\KERNEL.AC" -resolve

# Replace /mnt/c with C: for Windows paths if running in WSL
if ($IsLinux) {
    $bootAsm  = $bootAsm -replace "^/mnt/c", "C:"
    $stageAsm = $stageAsm -replace "^/mnt/c", "C:"
    $kernelAc = $kernelAc -replace "^/mnt/c", "C:"
}

# Stage 1: Assemble BOOTLOADER (first stage, 512-byte boot sector)
Write-Host "=== Assembling BOOTLOADER ==="
& $ASTRAC asm $bootAsm bits 16 org 7C00 debug arch i286
if ($LASTEXITCODE -ne 0) { Write-Host "BOOTLOADER failed, exit code $LASTEXITCODE"; exit 1 }

# Stage 2: Assemble SECOND_STAGE (16-bit → 32-bit transition)
Write-Host "=== Assembling SECOND_STAGE ==="
& $ASTRAC asm $stageAsm bits 16 org 7E00 debug
if ($LASTEXITCODE -ne 0) { Write-Host "SECOND_STAGE failed, exit code $LASTEXITCODE"; exit 1 }

# Stage 3: Compile KERNEL (32-bit AC kernel)
Write-Host "=== Compiling KERNEL ==="
& $ASTRAC comp $kernelAc bits 32 org 10000 entry _start debug
if ($LASTEXITCODE -ne 0) { Write-Host "KERNEL failed, exit code $LASTEXITCODE"; exit 1 }

# Stage 4: Create floppy image (1.44 MB = 1474560 bytes)
Write-Host "=== Creating floppy image ==="
$floppy = [byte[]]::new(1474560)

$boot   = [System.IO.File]::ReadAllBytes((Join-Path $scriptDir "..\BOOTLOADER\BOOTLOADER.BIN"))
$stage2 = [System.IO.File]::ReadAllBytes((Join-Path $scriptDir "..\BOOTLOADER\SECOND_STAGE.BIN"))
$kernel = [System.IO.File]::ReadAllBytes((Join-Path $scriptDir "..\KERNEL\KERNEL.BIN"))

# LBA 0 (sector 1)  → bootloader (must be exactly 512 bytes)
$boot.CopyTo($floppy, 0)

# LBA 1 (sector 2)  → second stage (8 sectors max = 4096 bytes)
$stage2.CopyTo($floppy, 512)

# LBA 9 (sector 10) → kernel (up to 18 sectors = 9216 bytes)
$kernel.CopyTo($floppy, 4608)

# If OUTPUT directory doesn't exist, create it
$outputDir = Join-Path $scriptDir "..\OUTPUT"
if (-not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir | Out-Null
}

[System.IO.File]::WriteAllBytes((Join-Path $scriptDir "..\OUTPUT\floppy.img"), $floppy)
Write-Host "floppy.img created ($($floppy.Length) bytes)"

# Show sizes
Write-Host "BOOTLOADER.BIN:     $($boot.Length) bytes"
Write-Host "SECOND_STAGE.BIN:   $($stage2.Length) bytes"
Write-Host "KERNEL.BIN:         $($kernel.Length) bytes"
Write-Host ""
Write-Host "QEMU command:"
Write-Host "  qemu-system-i386 -drive format=raw,file=$(Join-Path $scriptDir '..\OUTPUT\floppy.img'),if=floppy"

Pop-Location
