# Build + flash tab5-nc2000 (standalone NC2000 / NC1020 wqx emulator) to the
# M5Stack Tab5. Everything (rom/nor/nand/state) lives on the SD card, so there
# is no ROM data partition to flash — just the app.
#
# Usage:  .\tools\build_flash.ps1 [-Port COM4] [-NoFlash]
#   -NoFlash : build only, don't flash
param(
    [string]$Port = "COM4",
    [switch]$NoFlash
)
$ErrorActionPreference = "Stop"

# Activate ESP-IDF v5.5.2 (shell state does not persist between calls).
. C:\Espressif\frameworks\esp-idf-v5.5.2\export.ps1

$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$app  = Join-Path $root "apps\nc2000_tab5"

Set-Location $app

if ($NoFlash) {
    idf.py build
    return
}

idf.py -p $Port -b 460800 build flash
