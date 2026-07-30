#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace crypto_research::bitcoin {

[[nodiscard]] std::string p2pkh_address(std::span<const std::byte> public_key, bool mainnet = true);

// Converts a validated, internally-held scalar directly to a public P2PKH address.
// The scalar is neither serialized nor returned by this API.
[[nodiscard]] std::string p2pkh_address_from_secret_scalar(std::span<const std::byte> secret_scalar,
                                                            bool compressed = true,
                                                            bool mainnet = true);

} // namespace crypto_research::bitcoin
