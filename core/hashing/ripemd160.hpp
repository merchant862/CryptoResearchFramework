#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace crypto_research::hashing {

using Ripemd160Digest = std::array<std::byte, 20>;

[[nodiscard]] Ripemd160Digest ripemd160(std::span<const std::byte> data);

} // namespace crypto_research::hashing
