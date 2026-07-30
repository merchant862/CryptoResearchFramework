[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
cmake --build --preset windows-debug
if ($LASTEXITCODE -ne 0) { throw 'Test build failed.' }
ctest --test-dir build/windows-debug --output-on-failure
if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
