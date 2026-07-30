#include "bitcoin/p2pkh.hpp"

#include "bitcoin/private_key.hpp"
#include "core/encoding/base58.hpp"
#include "core/hashing/ripemd160.hpp"
#include "core/hashing/sha256.hpp"

#include <vector>

namespace crypto_research::bitcoin {

std::string p2pkh_address(std::span<const std::byte> public_key, bool mainnet) {
    const auto sha = hashing::sha256(public_key);
    const auto hash160 = hashing::ripemd160(sha);

    std::vector<std::byte> payload;
    payload.reserve(21);
    payload.push_back(mainnet ? std::byte{0x00} : std::byte{0x6f});
    payload.insert(payload.end(), hash160.begin(), hash160.end());
    return encoding::base58check_encode(payload);
}

std::string p2pkh_address_from_secret_scalar(std::span<const std::byte> secret_scalar,
                                             bool compressed,
                                             bool mainnet) {
    const auto internal_key = PrivateKey::from_secret_scalar(secret_scalar);
    return p2pkh_address(internal_key.public_key(compressed), mainnet);
}

} // namespace crypto_research::bitcoin
