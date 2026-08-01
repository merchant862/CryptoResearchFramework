#pragma once

#include <span>
#include <string>
#include <vector>

namespace crypto_research::bitcoin::address {

// BIP49 P2SH-wrapped SegWit address (3...).
// Public key must be 33-byte compressed.
[[nodiscard]] std::string p2sh_address(std::span<const std::byte> public_key);

// BIP84 Native SegWit P2WPKH address (bc1q...).
// Public key must be 33-byte compressed.
[[nodiscard]] std::string bech32_address(std::span<const std::byte> public_key);

// BIP350 Native SegWit P2WPKH address (bc1q...), same as bech32_address
// but uses the updated Bech32m checksum constant.
[[nodiscard]] std::string bech32m_address(std::span<const std::byte> public_key);

} // namespace crypto_research::bitcoin::address
