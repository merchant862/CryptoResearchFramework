[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
cmake --preset windows-debug
if ($LASTEXITCODE -ne 0) { throw 'Debug configuration failed.' }
cmake --build --preset windows-debug
if ($LASTEXITCODE -ne 0) { throw 'Debug build failed.' }
