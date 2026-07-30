# Codex Context

  Goal:
  Build a Windows-first C++20 cryptography research framework to solve BTC puzzle challenges and learn the full stack.

  Current Puzzle:
  privatekeys.pw 0.2 BTC puzzle.

  Target Address:
  1KfZGvwZxsvSmemoCmEV75uqcNzYBHjkHZ

  Important Findings:
  - `breathe` and `stop` are not BIP39 words.
  - `breathe` and `stop` are old Electrum words.
  - `food`, `liberty`, `proof`, `this` are not old Electrum words.
  - Image evidence and puzzle hints should be tracked by source, location, confidence, and transform.
  - Public address tools are limited to address-string validation, formatting, and list comparison.

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
  - safe evidence text reporting
  - public address reporting

  First C++ Phase:
  1. SHA-256
  2. RIPEMD-160
  3. Base58Check
  4. secp256k1 public key generation
  5. P2PKH address generation
  6. Unit tests
