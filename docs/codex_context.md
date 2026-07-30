# Codex Context

  Goal:
  Build a Windows-first C++20 cryptography research framework to solve BTC puzzle challenges and learn the full stack.

  Current Puzzle:
  privatekeys.pw 0.2 BTC puzzle.

  Target Address:
  1KfZGvwZxsvSmemoCmEV75uqcNzYBHjkHZ

  Address Details:
  - Type: Base58 P2PKH legacy Bitcoin address
  - Balance: 0.20107284 BTC
  - Total received: 0.20107284 BTC
  - Total sent: 0 BTC
  - Funded UTXOs: 5

  Important Findings:
  - `breathe` and `stop` are not BIP39 words.
  - `breathe` and `stop` are old Electrum words.
  - `food`, `liberty`, `proof`, `this` are not old Electrum words.
  - PNG has `sBIT 4`, so low 4 bits are suspicious.
  - `zsteg` found no clean seed/passphrase payload.
  - Simple BIP39 12/24-word candidates failed.
  - Electrum v2 obvious candidates failed.
  - Old Electrum is a strong branch but exact words/order still unclear.

  Existing Linux Scripts:
  - btc-puzzle-checker.py
  - btc-puzzle-parallel-checker.py
  - btc-puzzle-electrum-old-checker.py
  - btc-puzzle-brainwallet-checker.py
  - candidate generators

  Framework Plan:
  - C++20
  - Visual Studio 2022 Community
  - CMake
  - vcpkg
  - GoogleTest
  - OpenSSL
  - libsecp256k1
  - OpenCV
  - future CUDA

  First C++ Phase:
  1. SHA-256
  2. RIPEMD-160
  3. Base58Check
  4. secp256k1 public key generation
  5. P2PKH address generation
  6. Unit tests