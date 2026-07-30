#include "bitcoin/bip39/mnemonic.hpp"
#include "core/hashing/pbkdf2.hpp"
#include "core/utils/hex.hpp"

#include <gtest/gtest.h>

#include <array>
#include <span>
#include <string_view>
#include <vector>

namespace {

TEST(Bip39, ValidatesPublishedAllZeroEntropyVector) {
    std::vector<std::string_view> wordlist(2048U, "unused");
    wordlist[0] = "abandon";
    wordlist[3] = "about";
    const std::array<std::string_view, 12> words{
        "abandon", "abandon", "abandon", "abandon", "abandon", "abandon",
        "abandon", "abandon", "abandon", "abandon", "abandon", "about"};
    EXPECT_TRUE(crypto_research::bitcoin::bip39::checksum_is_valid(words, wordlist));
    const auto invalid_words = std::array<std::string_view, 12>{
        "abandon", "abandon", "abandon", "abandon", "abandon", "abandon",
        "abandon", "abandon", "abandon", "abandon", "abandon", "abandon"};
    EXPECT_FALSE(crypto_research::bitcoin::bip39::checksum_is_valid(invalid_words, wordlist));
}

TEST(Bip39, Pbkdf2MatchesPublishedTrezorVector) {
    constexpr std::string_view phrase = "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
    constexpr std::string_view salt = "mnemonicTREZOR";
    const auto password = std::as_bytes(std::span{phrase.data(), phrase.size()});
    const auto salt_bytes = std::as_bytes(std::span{salt.data(), salt.size()});
    const auto output = crypto_research::hashing::pbkdf2_hmac_sha512(password, salt_bytes, 2048U);
    EXPECT_EQ(crypto_research::utils::to_hex(output),
              "c55257c360c07c72029aebc1b53c05ed0362ada38ead3e3e9efa3708e53495531"
              "f09a6987599d18264c1e1c92f2cf141630c7a3c4ab7c81b2f001698e7463b04");
}

} // namespace
