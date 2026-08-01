#include "bitcoin/bip39/seed.hpp"

#include "core/hashing/pbkdf2.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

namespace crypto_research::bitcoin::bip39 {

    std::vector<std::byte> mnemonic_to_seed(std::span<const std::string_view> words,
        std::string_view passphrase) {
        // Join words with a single space.
        std::string mnemonic;
        for (std::size_t i = 0; i < words.size(); ++i) {
            if (i > 0) mnemonic.push_back(' ');
            mnemonic.append(words[i]);
        }

        const std::string salt = std::string{ "mnemonic" } + std::string{ passphrase };

        const auto mnemonic_bytes = std::as_bytes(std::span{ mnemonic });
        const auto salt_bytes = std::as_bytes(std::span{ salt });

        const auto output = hashing::pbkdf2_hmac_sha512(mnemonic_bytes, salt_bytes, 2048);
        return { output.begin(), output.end() };
    }

} // namespace crypto_research::bitcoin::bip39