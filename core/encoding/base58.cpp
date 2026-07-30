#include "core/encoding/base58.hpp"

#include "core/hashing/sha256.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>

namespace crypto_research::encoding {
namespace {

constexpr auto alphabet = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

} // namespace

std::string base58_encode(std::span<const std::byte> data) {
    std::size_t zeroes = 0;
    while (zeroes < data.size() && data[zeroes] == std::byte{0}) {
        ++zeroes;
    }

    // The base-256 to base-58 conversion avoids fixed-width integer limits.
    std::vector<unsigned char> digits((data.size() - zeroes) * 138 / 100 + 1);
    std::size_t length = 0;
    for (std::size_t index = zeroes; index < data.size(); ++index) {
        unsigned int carry = std::to_integer<unsigned char>(data[index]);
        std::size_t position = 0;
        for (auto cursor = digits.rbegin(); (carry != 0 || position < length) && cursor != digits.rend(); ++cursor, ++position) {
            carry += 256U * *cursor;
            *cursor = static_cast<unsigned char>(carry % 58U);
            carry /= 58U;
        }
        length = std::max(length, position);
    }

    auto first_digit = digits.end() - static_cast<std::ptrdiff_t>(length);
    std::string result(zeroes, '1');
    result.reserve(zeroes + length);
    for (; first_digit != digits.end(); ++first_digit) {
        result.push_back(alphabet[*first_digit]);
    }
    return result;
}

std::string base58check_encode(std::span<const std::byte> payload) {
    std::vector<std::byte> encoded(payload.begin(), payload.end());
    const auto checksum = hashing::double_sha256(payload);
    encoded.insert(encoded.end(), checksum.begin(), checksum.begin() + 4);
    return base58_encode(encoded);
}

std::vector<std::byte> base58check_decode(std::string_view encoded) {
    if (encoded.empty()) throw std::invalid_argument("Base58Check input is empty");
    std::size_t zeroes = 0;
    while (zeroes < encoded.size() && encoded[zeroes] == '1') ++zeroes;
    std::vector<unsigned char> decoded((encoded.size() - zeroes) * 733U / 1000U + 1U);
    std::size_t length = 0;
    for (std::size_t index = zeroes; index < encoded.size(); ++index) {
        const auto* found = std::char_traits<char>::find(alphabet, 58U, encoded[index]);
        if (found == nullptr) throw std::invalid_argument("Base58Check input contains an invalid character");
        unsigned int carry = static_cast<unsigned int>(found - alphabet);
        std::size_t position = 0;
        for (auto cursor = decoded.rbegin(); (carry != 0U || position < length) && cursor != decoded.rend(); ++cursor, ++position) {
            carry += 58U * *cursor;
            *cursor = static_cast<unsigned char>(carry & 0xffU);
            carry >>= 8U;
        }
        length = std::max(length, position);
    }
    const auto first = decoded.end() - static_cast<std::ptrdiff_t>(length);
    std::vector<std::byte> bytes(zeroes, std::byte{0});
    for (auto cursor = first; cursor != decoded.end(); ++cursor) bytes.push_back(std::byte{*cursor});
    if (bytes.size() < 5U) throw std::invalid_argument("Base58Check input is too short");
    const auto payload_size = bytes.size() - 4U;
    const auto checksum = hashing::double_sha256(std::span{bytes.data(), payload_size});
    if (!std::equal(bytes.begin() + static_cast<std::ptrdiff_t>(payload_size), bytes.end(), checksum.begin())) {
        throw std::invalid_argument("Base58Check checksum does not match");
    }
    bytes.resize(payload_size);
    return bytes;
}

} // namespace crypto_research::encoding
