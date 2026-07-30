#include "bitcoin/bip39/wordlist.hpp"

namespace crypto_research::bitcoin::bip39 {

EnglishWordlist load_english_wordlist(const std::filesystem::path& path) {
    return EnglishWordlist::load(path, 2048U);
}

} // namespace crypto_research::bitcoin::bip39
