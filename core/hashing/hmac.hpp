#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace crypto_research::hashing {

using HmacSha512Digest = std::array<std::byte, 64>;

[[nodiscard]] HmacSha512Digest hmac_sha512(std::span<const std::byte> key, std::span<const std::byte> message);

} // namespace crypto_research::hashing
