#include "bitcoin/private_key.hpp"

#include "core/utils/hex.hpp"

#include <secp256k1.h>

#include <algorithm>
#include <memory>
#include <stdexcept>

namespace crypto_research::bitcoin {
namespace {

struct ContextDeleter {
    void operator()(secp256k1_context* context) const noexcept { secp256k1_context_destroy(context); }
};

using Context = std::unique_ptr<secp256k1_context, ContextDeleter>;

Context make_context() {
    Context context{secp256k1_context_create(SECP256K1_CONTEXT_SIGN)};
    if (!context) throw std::runtime_error("could not create a libsecp256k1 context");
    return context;
}

} // namespace

PrivateKey::PrivateKey(std::array<std::byte, 32> bytes) noexcept : bytes_(bytes) {}

PrivateKey PrivateKey::from_hex(std::string_view hex) {
    const auto decoded = utils::from_hex(hex);
    return from_secret_scalar(decoded);
}

PrivateKey PrivateKey::from_secret_scalar(std::span<const std::byte> scalar) {
    if (scalar.size() != 32U) throw std::invalid_argument("a secp256k1 secret scalar must be exactly 32 bytes");

    std::array<std::byte, 32> bytes{};
    std::copy(scalar.begin(), scalar.end(), bytes.begin());
    const auto context = make_context();
    if (secp256k1_ec_seckey_verify(context.get(), reinterpret_cast<const unsigned char*>(bytes.data())) != 1) {
        throw std::invalid_argument("private key is outside the valid secp256k1 range");
    }
    return PrivateKey{bytes};
}

std::vector<std::byte> PrivateKey::public_key(bool compressed) const {
    const auto context = make_context();
    secp256k1_pubkey public_key{};
    if (secp256k1_ec_pubkey_create(context.get(), &public_key, reinterpret_cast<const unsigned char*>(bytes_.data())) != 1) {
        throw std::runtime_error("libsecp256k1 could not derive the public key");
    }

    std::vector<std::byte> serialized(compressed ? 33U : 65U);
    std::size_t size = serialized.size();
    const unsigned int flags = compressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED;
    if (secp256k1_ec_pubkey_serialize(context.get(), reinterpret_cast<unsigned char*>(serialized.data()), &size, &public_key, flags) != 1) {
        throw std::runtime_error("libsecp256k1 could not serialize the public key");
    }
    serialized.resize(size);
    return serialized;
}

} // namespace crypto_research::bitcoin
