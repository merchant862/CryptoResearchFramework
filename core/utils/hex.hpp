#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace crypto_research::utils {

[[nodiscard]] std::vector<std::byte> from_hex(std::string_view hex);
[[nodiscard]] std::string to_hex(std::span<const std::byte> bytes);

} // namespace crypto_research::utils
