#pragma once

namespace crypto_research::cli {

// Brute-force BIP39 mnemonics from a candidate word pool.
// Usage: crypto_research brute-force --words <file> --target <address>
//        [--path bip44|bip49|bip84] [--passphrases <file>] [--workers <n>]
int brute_force_command(int argc, char* argv[]);

} // namespace crypto_research::cli
