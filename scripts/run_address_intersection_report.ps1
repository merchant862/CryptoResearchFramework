param([Parameter(Mandatory=$true)][string]$Left,[Parameter(Mandatory=$true)][string]$Right,[Parameter(Mandatory=$true)][string]$JsonReport,[Parameter(Mandatory=$true)][string]$Csv)
$ErrorActionPreference='Stop'; $root=Split-Path -Parent $PSScriptRoot; Set-Location $root
cmake --build --preset windows-debug --target address_intersection_report; if($LASTEXITCODE -ne 0){throw 'Build failed.'}
& "$root\build\windows-debug\Debug\address_intersection_report.exe" --left $Left --right $Right --json-report $JsonReport --csv $Csv
