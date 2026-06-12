# Merge bootloader + partition-table + otadata + app into ONE image that flashes
# to offset 0x0 — for use with M5Burner, the ESP web flasher, or a plain
# `esptool write_flash 0x0 tab5-nc2000-merged.bin`.
# Produces: tab5-nc2000-merged.bin in the project root.
$ErrorActionPreference = "Stop"
. C:\Espressif\frameworks\esp-idf-v5.5.2\export.ps1 *> $null
$root = Resolve-Path (Join-Path $PSScriptRoot "..")
$app  = Join-Path $root "apps\nc2000_tab5"
$out  = Join-Path $root "tab5-nc2000-merged.bin"
Set-Location $app
idf.py build
idf.py merge-bin -o $out
Write-Host "`nMerged image: $out  (flash to offset 0x0)"
