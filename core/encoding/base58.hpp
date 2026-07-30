#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace crypto_research::encoding {

[[nodiscard]] std::string base58_encode(std::span<const std::byte> data);
[[nodiscard]] std::string base58check_encode(std::span<const std::byte> payload);
[[nodiscard]] std::vector<std::byte> base58check_decode(std::string_view encoded);

} // namespace crypto_research::encoding
