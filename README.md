# Crypto Research Framework

Windows-first C++20 research toolkit for public Bitcoin address checks, PNG evidence analysis, low-bit/bit-plane inspection, safe benchmarks, and local evidence text reporting.

This repository is for reproducible puzzle/evidence analysis. The included tools do not automate seed phrase recovery, private-key recovery, wallet access, balance queries, or fund movement.

## What Is Included

- Bitcoin hashing and encoding primitives: SHA-256, RIPEMD-160, Base58Check.
- secp256k1 public-key and P2PKH address support.
- Local BIP39 and old Electrum wordlist loaders for word/index lookup.
- xpub-only normal-child BIP32 derivation.
- PNG evidence reports with chunks, residual images, bit-plane exports, and printable low-bit ASCII runs.
- Public address list matching, address format reports, and address intersection reports.
- Evidence text term-frequency reports for extracted text or manual notes.
- Windows PowerShell scripts for build, test, reports, benchmarks, and packaging.

## Requirements

- Windows 11 64-bit.
- Visual Studio 2022 Community with the "Desktop development with C++" workload.
- CMake 3.25 or newer.
- vcpkg.
- Optional: NVIDIA CUDA Toolkit 13.3 for CUDA PNG extraction builds.

Set `VCPKG_ROOT` before configuring:

```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
```

Dependencies are declared in [vcpkg.json](vcpkg.json): OpenSSL, secp256k1, OpenCV, spdlog, nlohmann-json, and GTest.

## Quick Start

From the repository root:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Equivalent helper scripts:

```powershell
.\scripts\build_debug.ps1
.\scripts\run_tests.ps1
```

Debug executables are written under:

```text
.\build\windows-debug\Debug\
```

## CUDA Build

Use this only if CUDA Toolkit 13.3 is installed at the path configured in [CMakePresets.json](CMakePresets.json).

```powershell
cmake --preset windows-cuda-debug
cmake --build --preset windows-cuda-debug
```

CUDA output goes under:

```text
.\build\windows-cuda-debug\Debug\
```

## Puzzle Evidence Workflow

Run the richer PNG report first:

```powershell
.\scripts\run_png_report.ps1 `
  -Image .\evidence\0.2-btc-puzzle.png `
  -Output .\analysis-output\png-report
```

Important outputs:

- `report.json`: image metadata, PNG chunks, artefacts, and ASCII run summary.
- `extracted_text.txt`: printable ASCII runs from RGB/BGR, bit-depth 1-4, XY/YX scans.
- `low1_residual.png` through `low4_residual.png`: low-bit residual exports.
- `bitplanes\all_channels_bitplanes.png`: bit-plane contact sheet.

Then summarize extracted text:

```powershell
.\scripts\run_evidence_text_report.ps1 `
  -Input .\analysis-output\png-report\extracted_text.txt `
  -JsonReport .\reports\evidence_text.json `
  -Csv .\reports\evidence_text.csv
```

You can also run it on manual notes:

```powershell
.\scripts\run_evidence_text_report.ps1 `
  -Input .\examples\text\evidence_notes.txt `
  -JsonReport .\reports\evidence_notes.json `
  -Csv .\reports\evidence_notes.csv
```

## Public Address Tools

These tools only process public address strings.

Match a target against a public address list:

```powershell
.\build\windows-debug\Debug\public_address_list_matcher.exe `
  --addresses .\examples\text\public_addresses.txt `
  --target 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
```

Classify and validate address formats:

```powershell
.\scripts\run_address_format_report.ps1 `
  -Input .\examples\text\public_addresses.txt `
  -JsonReport .\reports\address_format.json `
  -Csv .\reports\address_format.csv
```

Intersect two public address lists:

```powershell
.\scripts\run_address_intersection_report.ps1 `
  -Left .\addresses_a.txt `
  -Right .\addresses_b.txt `
  -JsonReport .\reports\intersection.json `
  -Csv .\reports\intersection.csv
```

`address_format_report` validates Base58Check for P2PKH/P2SH and Bech32/Bech32m checksums for Bitcoin SegWit addresses. `address_intersection_report` records all matching line numbers, including duplicates.

## Main CLI

Derive a public P2PKH address from an explicit scalar input:

```powershell
.\build\windows-debug\Debug\crypto_research.exe 0000000000000000000000000000000000000000000000000000000000000001
```

Use `--uncompressed` for legacy uncompressed public-key format.

Basic PNG analysis is also available through the main CLI:

```powershell
.\build\windows-debug\Debug\crypto_research.exe analyze-image .\evidence\0.2-btc-puzzle.png .\analysis-output\basic
```

Batch PNG analysis:

```powershell
.\build\windows-debug\Debug\crypto_research.exe batch-analyze .\evidence .\analysis-output\batch --workers 8
```

## Benchmarks

```powershell
.\scripts\run_benchmarks.ps1
```

The benchmark executable measures hashing, Base58Check, secp256k1 public-key generation, P2PKH address creation, BIP39 checksum validation, and image low-bit extraction throughput.

## Package

Debug package:

```powershell
cmake --install build/windows-debug --config Debug --prefix .\dist
cpack --config build/windows-debug\CPackConfig.cmake -C Debug
```

Release package:

```powershell
.\scripts\build_release.ps1
cmake --install build/windows-release --config Release --prefix .\dist-release
cpack --config build/windows-release\CPackConfig.cmake -C Release
```

## Repository Notes

- [docs/usage.md](docs/usage.md) has shorter command examples.
- [docs/puzzle_workflow.md](docs/puzzle_workflow.md) tracks the current evidence workflow.
- [docs/codex_context.md](docs/codex_context.md) records project context.
- [evidence/0.2-btc-puzzle.png](evidence/0.2-btc-puzzle.png) is the checked-in puzzle image mirror.

## Safety Scope

Allowed project scope:

- image metadata and steganography evidence reports
- OCR/extracted text organization
- public address validation and comparison
- local wordlist lookup
- build, test, benchmark, and packaging work

Out of scope for this repository:

- seed phrase candidate generation
- mnemonic validation pipelines
- private-key recovery
- wallet derivation or balance checking
- network calls to blockchain services
