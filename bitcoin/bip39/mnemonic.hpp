#pragma once

#include <span>
#include <string_view>

namespace crypto_research::bitcoin::bip39 {

// Validates a mnemonic against a caller-supplied canonical BIP39 word list.
// It does not derive addresses, keys, or expose seed material.
[[nodiscard]] bool checksum_is_valid(std::span<const std::string_view> words,
                                     std::span<const std::string_view> wordlist);

} // namespace crypto_research::bitcoin::bip39
