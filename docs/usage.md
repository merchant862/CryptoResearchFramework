# Offline research framework (Windows)

## Build and test

From the project root in PowerShell:

```powershell
.\scripts\build_debug.ps1
.\scripts\run_tests.ps1
```

For a CUDA PNG analysis build, use `run_0_2_puzzle_analysis.ps1 -UseGpu` with a local image asset.

## Safe benchmarks

```powershell
.\scripts\run_benchmarks.ps1
```

## Local word lists

The BIP39 loader accepts a caller-supplied local English list containing exactly 2048 unique lines. The old Electrum research loader similarly requires exactly 1626 lines. Both expose word/index lookup only; neither command accesses a network.

## PNG and steganography research

```powershell
.\build\windows-cuda-debug\Debug\crypto_research.exe analyze-image <image.png> <output-folder> --gpu
```

The report includes PNG chunk metadata, bit-plane exports, low-bit data, and GPU timing where enabled. LSB utilities support RGB/BGR channel orders, bit depths 1–4, XY/YX traversal, and printable-ASCII forensic runs.

## Public address list comparison

```powershell
.\build\windows-cuda-debug\Debug\public_address_list_matcher.exe `
  --addresses .\examples\text\public_addresses.txt `
  --target 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH
```

This utility compares already-public address strings only. It does not derive or access wallet material.

## Public address reports

```powershell
.\build\windows-cuda-debug\Debug\address_format_report.exe --input .\addresses.txt --json-report .\reports\format.json --csv .\reports\format.csv
.\build\windows-cuda-debug\Debug\address_intersection_report.exe --left .\left.txt --right .\right.txt --json-report .\reports\intersection.json --csv .\reports\intersection.csv
```

Both tools accept public address strings only. They do not accept seed phrases or private keys, derive keys, or perform network calls.

## Evidence text reports

```powershell
.\scripts\run_evidence_text_report.ps1 -Input .\analysis-output\png-report\extracted_text.txt -JsonReport .\reports\evidence_text.json -Csv .\reports\evidence_text.csv
```

This utility tokenizes local evidence text, counts repeated terms, classifies words/numbers/alphanumeric tokens, and records source line numbers. It is evidence triage only; it does not generate mnemonic candidates, validate seed phrases, derive wallet keys, or query balances.

## Local package

```powershell
cmake --install build/windows-cuda-debug --config Debug --prefix .\dist
cpack --config build/windows-cuda-debug\CPackConfig.cmake -C Debug
```

The package contains only safe executables, documentation, and neutral examples.
