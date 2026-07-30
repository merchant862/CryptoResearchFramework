[CmdletBinding()]
param(
    [switch]$UseGpu
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$image = Join-Path $root 'evidence\0.2-btc-puzzle.png'
$output = Join-Path $root 'analysis-output\0.2-btc-puzzle'
$env:VCPKG_DOWNLOADS = Join-Path $root '.vcpkg-downloads'
New-Item -ItemType Directory -Force $env:VCPKG_DOWNLOADS | Out-Null
$cudaCompiler = 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3\bin\nvcc.exe'
if ($UseGpu -and -not (Test-Path -LiteralPath $cudaCompiler)) {
    throw 'CUDA 13.3 compiler was not found. Complete the CUDA Toolkit installation first.'
}
if ($UseGpu) {
    $cudaRoot = Split-Path -Parent (Split-Path -Parent $cudaCompiler)
    $env:CUDACXX = $cudaCompiler
    $env:CUDA_PATH = $cudaRoot
    $env:CUDA_PATH_V13_3 = $cudaRoot
}

if (-not (Test-Path -LiteralPath $image)) {
    throw "Puzzle image not found: $image"
}

if (-not $env:VCPKG_ROOT) {
    $vcpkgCandidates = @(
        'C:\vcpkg',
        'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\vcpkg',
        'C:\Program Files\Microsoft Visual Studio\18\Community\VC\vcpkg'
    )
    $env:VCPKG_ROOT = $vcpkgCandidates | Where-Object { Test-Path (Join-Path $_ 'scripts\buildsystems\vcpkg.cmake') } | Select-Object -First 1
}
if (-not $env:VCPKG_ROOT) {
    throw 'Set VCPKG_ROOT to a vcpkg checkout before running this script.'
}

$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    $cmakeCandidates = @(
        'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe',
        'C:\Program Files\Microsoft Visual Studio\18\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    )
    $cmake = $cmakeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
} else {
    $cmake = $cmake.Source
}

if ($UseGpu) {
    $vs2022Cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
    if (-not (Test-Path -LiteralPath $vs2022Cmake)) {
        throw 'GPU builds require Visual Studio 2022 C++ Build Tools and CUDA Toolkit 13.1. Open a VS 2022 Developer PowerShell after installing them.'
    }
    $cmake = $vs2022Cmake
}
if (-not $cmake) {
    throw 'CMake was not found. Install the Visual Studio C++ desktop workload with CMake tools.'
}

$usesVs2026 = $cmake -like '*Visual Studio\18\*'
if ($usesVs2026) {
    $preset = if ($UseGpu) { 'windows-vs2026-cuda-debug' } else { 'windows-vs2026-debug' }
    $binaryDirectory = if ($UseGpu) { 'build\windows-vs2026-cuda-debug' } else { 'build\windows-vs2026-debug' }
} else {
    $preset = if ($UseGpu) { 'windows-cuda-debug' } else { 'windows-debug' }
    $binaryDirectory = if ($UseGpu) { 'build\windows-cuda-debug' } else { 'build\windows-debug' }
}
& $cmake --preset $preset
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
& $cmake --build --preset $preset
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }

$binary = Join-Path $root "$binaryDirectory\Debug\crypto_research.exe"
$arguments = @('analyze-image', $image, $output)
if ($UseGpu) { $arguments += '--gpu' }
& $binary @arguments
if ($LASTEXITCODE -ne 0) { throw 'Puzzle analysis failed.' }

Write-Host "Analysis complete: $output"
