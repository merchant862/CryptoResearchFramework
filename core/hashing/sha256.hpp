#pragma once

#include <array>
#include <cstddef>
#include <span>

namespace crypto_research::hashing {

using Sha256Digest = std::array<std::byte, 32>;

[[nodiscard]] Sha256Digest sha256(std::span<const std::byte> data);
[[nodiscard]] Sha256Digest double_sha256(std::span<const std::byte> data);

} // namespace crypto_research::hashing
