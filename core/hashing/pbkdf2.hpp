#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace crypto_research::hashing {

using Pbkdf2Sha512Output = std::array<std::byte, 64>;

// Generic KDF primitive used only by deterministic cryptographic test vectors.
[[nodiscard]] Pbkdf2Sha512Output pbkdf2_hmac_sha512(std::span<const std::byte> password,
                                                     std::span<const std::byte> salt,
                                                     unsigned int iterations);

} // namespace crypto_research::hashing
