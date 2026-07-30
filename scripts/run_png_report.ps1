[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Image,
    [Parameter(Mandatory = $true)][string]$Output
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
cmake --build --preset windows-debug --target png_report
if ($LASTEXITCODE -ne 0) { throw 'PNG tool build failed.' }
& "$root\build\windows-debug\Debug\png_report.exe" --image $Image --out $Output
