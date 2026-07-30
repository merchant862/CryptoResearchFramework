#include "bitcoin/electrum/old_wordlist.hpp"

namespace crypto_research::bitcoin::electrum {

OldWordlist load_old_wordlist(const std::filesystem::path& path) {
    return OldWordlist::load(path, 1626U);
}

} // namespace crypto_research::bitcoin::electrum
