# Build multi-stage bootloader + kernel floppy image
# Requires: AstraC.exe in build\Release\

$scriptDir = $PSScriptRoot
Push-Location $scriptDir

# Paths
$ASTRAC = Join-Path $scriptDir "..\build\Release\AstraC.exe"
$bootAsm  = Join-Path $scriptDir "BOOTLOADER.AS"
$stageAsm = Join-Path $scriptDir "SECOND_STAGE.AS"
$kernelAc = Join-Path $scriptDir "KERNEL.AC"

# Stage 1: Assemble BOOTLOADER (first stage, 512-byte boot sector)
Write-Host "=== Assembling BOOTLOADER ==="
& $ASTRAC asm $bootAsm bits 16 org 7C00
if ($LASTEXITCODE -ne 0) { Write-Host "BOOTLOADER failed"; exit 1 }

# Stage 2: Assemble SECOND_STAGE (16-bit → 32-bit transition)
Write-Host "=== Assembling SECOND_STAGE ==="
& $ASTRAC asm $stageAsm bits 16 org 7E00
if ($LASTEXITCODE -ne 0) { Write-Host "SECOND_STAGE failed"; exit 1 }

# Stage 3: Compile KERNEL (32-bit AC kernel)
Write-Host "=== Compiling KERNEL ==="
& $ASTRAC comp $kernelAc bits 32 org 10000 verbose warn err
if ($LASTEXITCODE -ne 0) { Write-Host "KERNEL failed"; exit 1 }

# Stage 4: Create floppy image (1.44 MB = 1474560 bytes)
Write-Host "=== Creating floppy image ==="
$floppy = [byte[]]::new(1474560)

$boot   = [System.IO.File]::ReadAllBytes((Join-Path $scriptDir "BOOTLOADER.BIN"))
$stage2 = [System.IO.File]::ReadAllBytes((Join-Path $scriptDir "SECOND_STAGE.BIN"))
$kernel = [System.IO.File]::ReadAllBytes((Join-Path $scriptDir "KERNEL.BIN"))

# LBA 0 (sector 1)  → bootloader (must be exactly 512 bytes)
$boot.CopyTo($floppy, 0)

# LBA 1 (sector 2)  → second stage (8 sectors max = 4096 bytes)
$stage2.CopyTo($floppy, 512)

# LBA 9 (sector 10) → kernel (up to 9 sectors = 4608 bytes)
$kernel.CopyTo($floppy, 4608)

[System.IO.File]::WriteAllBytes((Join-Path $scriptDir "floppy.img"), $floppy)
Write-Host "floppy.img created ($($floppy.Length) bytes)"

# Show sizes
Write-Host "BOOTLOADER.BIN:     $($boot.Length) bytes"
Write-Host "SECOND_STAGE.BIN:   $($stage2.Length) bytes"
Write-Host "KERNEL.BIN:         $($kernel.Length) bytes"
Write-Host ""
Write-Host "QEMU command:"
Write-Host "  qemu-system-i386 -drive format=raw,file=$(Join-Path $scriptDir 'floppy.img'),if=floppy"

Pop-Location
