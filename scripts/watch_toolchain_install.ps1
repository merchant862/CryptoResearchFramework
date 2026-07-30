[CmdletBinding()]
param(
    [int]$IntervalSeconds = 30,
    [int]$MaxChecks = 480
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$statusPath = Join-Path $root 'installers\toolchain-status.json'
$vsCmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$cudaNvcc = 'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v13.3\bin\nvcc.exe'

for ($check = 1; $check -le $MaxChecks; ++$check) {
    $processes = @(Get-Process vs_community_2022,setup,vs_installer,vs_setup_bootstrapper -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty ProcessName -Unique)
    $state = [ordered]@{
        checked_at_utc = (Get-Date).ToUniversalTime().ToString('o')
        check = $check
        visual_studio_2022_ready = Test-Path -LiteralPath $vsCmake
        cuda_13_3_ready = Test-Path -LiteralPath $cudaNvcc
        active_installer_processes = $processes
        status = if ((Test-Path -LiteralPath $vsCmake) -and (Test-Path -LiteralPath $cudaNvcc)) { 'ready' } elseif ($processes.Count -gt 0) { 'installing' } else { 'waiting_for_installation' }
    }
    $state | ConvertTo-Json | Set-Content -LiteralPath $statusPath -Encoding utf8
    if ($state.status -eq 'ready') { exit 0 }
    Start-Sleep -Seconds $IntervalSeconds
}

exit 1
