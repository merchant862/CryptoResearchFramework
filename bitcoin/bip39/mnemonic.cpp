#include "bitcoin/bip39/mnemonic.hpp"

#include "core/hashing/sha256.hpp"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <vector>

namespace crypto_research::bitcoin::bip39 {
namespace {

bool supported_word_count(std::size_t count) {
    return count == 12U || count == 15U || count == 18U || count == 21U || count == 24U;
}

} // namespace

bool checksum_is_valid(std::span<const std::string_view> words, std::span<const std::string_view> wordlist) {
    if (!supported_word_count(words.size()) || wordlist.size() != 2048U) return false;

    const auto checksum_bit_count = words.size() / 3U;
    const auto entropy_bit_count = words.size() * 11U - checksum_bit_count;
    std::vector<std::byte> entropy(entropy_bit_count / 8U);
    std::vector<bool> bits;
    bits.reserve(words.size() * 11U);
    for (const auto word : words) {
        const auto iterator = std::find(wordlist.begin(), wordlist.end(), word);
        if (iterator == wordlist.end()) return false;
        const auto index = static_cast<unsigned int>(std::distance(wordlist.begin(), iterator));
        for (int bit = 10; bit >= 0; --bit) bits.push_back(((index >> bit) & 1U) != 0U);
    }
    for (std::size_t bit = 0; bit < entropy_bit_count; ++bit) {
        if (bits[bit]) entropy[bit / 8U] |= std::byte{static_cast<unsigned char>(1U << (7U - (bit % 8U)))};
    }
    const auto digest = hashing::sha256(entropy);
    for (std::size_t bit = 0; bit < checksum_bit_count; ++bit) {
        const auto expected = (std::to_integer<unsigned char>(digest[bit / 8U]) >> (7U - (bit % 8U))) & 1U;
        if (bits[entropy_bit_count + bit] != (expected != 0U)) return false;
    }
    return true;
}

} // namespace crypto_research::bitcoin::bip39
