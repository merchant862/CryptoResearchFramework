#pragma once

#include "core/wordlists/local_wordlist.hpp"

namespace crypto_research::bitcoin::electrum {

using OldWordlist = wordlists::LocalWordlist;

[[nodiscard]] OldWordlist load_old_wordlist(const std::filesystem::path& path);

} // namespace crypto_research::bitcoin::electrum
