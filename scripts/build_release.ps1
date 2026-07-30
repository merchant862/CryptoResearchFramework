[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
cmake --preset windows-release
if ($LASTEXITCODE -ne 0) { throw 'Release configuration failed.' }
cmake --build --preset windows-release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }
