#include "core/utils/hex.hpp"

#include <stdexcept>

namespace crypto_research::utils {
namespace {

unsigned char nibble(char value) {
    if (value >= '0' && value <= '9') return static_cast<unsigned char>(value - '0');
    if (value >= 'a' && value <= 'f') return static_cast<unsigned char>(value - 'a' + 10);
    if (value >= 'A' && value <= 'F') return static_cast<unsigned char>(value - 'A' + 10);
    throw std::invalid_argument("hex string contains a non-hexadecimal character");
}

} // namespace

std::vector<std::byte> from_hex(std::string_view hex) {
    if (hex.size() % 2 != 0) throw std::invalid_argument("hex string must contain an even number of characters");
    std::vector<std::byte> result;
    result.reserve(hex.size() / 2);
    for (std::size_t index = 0; index < hex.size(); index += 2) {
        result.push_back(std::byte{static_cast<unsigned char>((nibble(hex[index]) << 4U) | nibble(hex[index + 1]))});
    }
    return result;
}

std::string to_hex(std::span<const std::byte> bytes) {
    constexpr std::string_view digits = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const auto byte : bytes) {
        const auto value = std::to_integer<unsigned char>(byte);
        result.push_back(digits[value >> 4U]);
        result.push_back(digits[value & 0x0FU]);
    }
    return result;
}

} // namespace crypto_research::utils
