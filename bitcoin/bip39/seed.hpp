#pragma once

#include "bitcoin/bip39/mnemonic.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace crypto_research::bitcoin::bip39 {

// Converts a validated BIP39 mnemonic phrase into a 64-byte seed using
// PBKDF2-HMAC-SHA512 ("mnemonic" + optional passphrase as salt, 2048 iterations).
// The caller should validate the mnemonic with checksum_is_valid() first.
[[nodiscard]] std::vector<std::byte> mnemonic_to_seed(std::span<const std::string_view> words,
                                                      std::string_view passphrase = "");

} // namespace crypto_research::bitcoin::bip39
