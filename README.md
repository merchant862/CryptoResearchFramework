# Crypto Research Framework

A Windows-first, C++20 framework for cryptography research. It provides public Bitcoin address tooling, safe benchmarks, and PNG evidence analysis for reproducible puzzle research.

## Prerequisites

- Visual Studio 2022 with the Desktop development with C++ workload
- CMake 3.25 or later
- vcpkg

## Build on Windows

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
ctest --preset windows-debug
```

Set `VCPKG_ROOT` to your vcpkg checkout before configuring. The configure preset uses its CMake toolchain file. `windows-debug` targets Visual Studio 2022; `windows-vs2026-debug` is included for the currently installed Visual Studio 2026 generator.

## CLI

```powershell
.\build\windows-debug\Debug\crypto_research.exe 0000000000000000000000000000000000000000000000000000000000000001
```

By default it creates a compressed public key and its mainnet P2PKH address. Pass `--uncompressed` to use the legacy uncompressed public-key format.

## Public address utilities

These tools only process public address strings. They do not handle seed phrases, private keys, wallet files, network queries, or fund movement.

```powershell
.\build\windows-debug\Debug\public_address_list_matcher.exe `
  --addresses .\addresses.txt `
  --target 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH

.\build\windows-debug\Debug\address_format_report.exe `
  --input .\addresses.txt `
  --json-report .\reports\address_format.json `
  --csv .\reports\address_format.csv

.\build\windows-debug\Debug\address_intersection_report.exe `
  --left .\addresses_a.txt `
  --right .\addresses_b.txt `
  --json-report .\reports\intersection.json `
  --csv .\reports\intersection.csv
```

`address_format_report` classifies P2PKH, P2SH, and Bitcoin Bech32/Bech32m SegWit addresses, validates checksums, and writes JSON plus CSV. `address_intersection_report` reports matching public addresses across two lists with all matching line numbers.

## Evidence text utilities

```powershell
.\scripts\run_evidence_text_report.ps1 `
  -Input .\analysis-output\png-report\extracted_text.txt `
  -JsonReport .\reports\evidence_text.json `
  -Csv .\reports\evidence_text.csv
```

`evidence_text_report` tokenizes local notes or extracted text, counts repeated terms, classifies word/number/alphanumeric tokens, and records source line numbers. It is evidence triage only and does not generate mnemonic candidates or derive wallet material.

## Puzzle evidence analysis

```powershell
.\build\windows-debug\Debug\crypto_research.exe analyze-image .\evidence.png .\analysis-output
```

The command records PNG `sBIT` metadata, a low-nibble histogram, and writes the low four bits of every decoded image byte to `low4.raw`. This preserves evidence for reproducible analysis without mutating the source image.

For richer standalone reports, use:

```powershell
.\build\windows-debug\Debug\png_report.exe `
  --image .\evidence\0.2-btc-puzzle.png `
  --out .\analysis-output\png-report
```

It writes PNG chunk metadata, ASCII low-bit runs, bit-plane images, contact sheets, and JSON report output.

The checked-in public puzzle mirror is [evidence/0.2-btc-puzzle.png](evidence/0.2-btc-puzzle.png). Run the included script to configure, build, and analyse it in one step:

```powershell
.\scripts\run_0_2_puzzle_analysis.ps1
```

For a directory of PNGs, the framework uses a bounded CPU thread pool, live progress logs, per-image reports, and a `batch-report.json` timing summary:

```powershell
.\build\windows-debug\Debug\crypto_research.exe batch-analyze .\evidence .\analysis-output\batch --workers 8
```

For an NVIDIA GPU with the CUDA Toolkit installed, configure the CUDA preset and request GPU extraction:

```powershell
cmake --preset windows-cuda-debug
cmake --build --preset windows-cuda-debug
.\build\windows-cuda-debug\Debug\crypto_research.exe analyze-image .\evidence.png .\analysis-output --gpu
```

GPU builds require Visual Studio 2022 C++ Build Tools and NVIDIA CUDA Toolkit 13.1 or later. The installed CUDA 12.6 toolkit does not support the Visual Studio 2026 compiler toolset.
