#include "bitcoin/bip39/wordlist.hpp"
#include "bitcoin/electrum/old_wordlist.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path write_wordlist(std::string_view name, std::size_t count) {
    const auto path = std::filesystem::temp_directory_path() / std::string{name};
    std::ofstream output(path, std::ios::trunc);
    for (std::size_t index = 0; index < count; ++index) {
        if (index == 0) output << "abandon\n";
        else if (index == 3) output << "about\n";
        else if (index + 1U == count) output << "zoo\n";
        else output << "test-word-" << index << '\n';
    }
    return path;
}

TEST(Wordlist, Bip39LoadsExactly2048WordsAndSupportsBothLookups) {
    const auto path = write_wordlist("crypto-research-bip39-test.txt", 2048U);
    const auto wordlist = crypto_research::bitcoin::bip39::load_english_wordlist(path);
    EXPECT_EQ(wordlist.size(), 2048U);
    EXPECT_EQ(wordlist.index_of("abandon"), 0U);
    EXPECT_EQ(wordlist.index_of("about"), 3U);
    EXPECT_EQ(wordlist.word_at(0U), "abandon");
    EXPECT_EQ(wordlist.word_at(2047U), "zoo");
    EXPECT_FALSE(wordlist.index_of("not-a-bip39-word").has_value());
    std::filesystem::remove(path);
}

TEST(Wordlist, Bip39RejectsIncorrectWordCount) {
    const auto path = write_wordlist("crypto-research-short-bip39-test.txt", 12U);
    EXPECT_THROW((void)crypto_research::bitcoin::bip39::load_english_wordlist(path), std::invalid_argument);
    std::filesystem::remove(path);
}

TEST(Wordlist, OldElectrumLoadsExactly1626Words) {
    const auto path = write_wordlist("crypto-research-old-electrum-test.txt", 1626U);
    const auto wordlist = crypto_research::bitcoin::electrum::load_old_wordlist(path);
    EXPECT_EQ(wordlist.size(), 1626U);
    EXPECT_EQ(wordlist.index_of("about"), 3U);
    EXPECT_EQ(wordlist.word_at(1625U), "zoo");
    EXPECT_FALSE(wordlist.index_of("unknown").has_value());
    std::filesystem::remove(path);
}

} // namespace
