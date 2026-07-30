[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][Alias('Input')][string]$InputPath,
    [Parameter(Mandatory = $true)][string]$JsonReport,
    [Parameter(Mandatory = $true)][string]$Csv
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
cmake --build --preset windows-debug --target evidence_text_report
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
& "$root\build\windows-debug\Debug\evidence_text_report.exe" --input $InputPath --json-report $JsonReport --csv $Csv
