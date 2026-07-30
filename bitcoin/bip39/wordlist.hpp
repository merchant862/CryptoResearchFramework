#pragma once

#include "core/wordlists/local_wordlist.hpp"

namespace crypto_research::bitcoin::bip39 {

using EnglishWordlist = wordlists::LocalWordlist;

[[nodiscard]] EnglishWordlist load_english_wordlist(const std::filesystem::path& path);

} // namespace crypto_research::bitcoin::bip39
