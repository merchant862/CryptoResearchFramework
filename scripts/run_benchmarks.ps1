[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root
cmake --build --preset windows-debug --target crypto_research_safe_benchmarks
if ($LASTEXITCODE -ne 0) { throw 'Benchmark build failed.' }
& "$root\build\windows-debug\Debug\crypto_research_safe_benchmarks.exe"
