#include "bitcoin/bip32/xpub.hpp"

#include "bitcoin/p2pkh.hpp"
#include "core/encoding/base58.hpp"
#include "core/hashing/hmac.hpp"
#include "core/hashing/ripemd160.hpp"
#include "core/hashing/sha256.hpp"

#include <secp256k1.h>

#include <algorithm>
#include <memory>
#include <span>
#include <stdexcept>

namespace crypto_research::bitcoin::bip32 {
namespace {
constexpr std::array<std::byte, 4> mainnet_xpub_version{std::byte{0x04}, std::byte{0x88}, std::byte{0xb2}, std::byte{0x1e}};

struct ContextDeleter { void operator()(secp256k1_context* context) const noexcept { secp256k1_context_destroy(context); } };
using Context = std::unique_ptr<secp256k1_context, ContextDeleter>;
Context make_context() {
    Context context{secp256k1_context_create(SECP256K1_CONTEXT_VERIFY)};
    if (!context) throw std::runtime_error("could not create a libsecp256k1 context");
    return context;
}
void write_u32(std::array<std::byte, 4>& output, std::uint32_t value) {
    for (int position = 3; position >= 0; --position) { output[static_cast<std::size_t>(position)] = std::byte{static_cast<unsigned char>(value & 0xffU)}; value >>= 8U; }
}
}

ExtendedPublicKey ExtendedPublicKey::parse(std::string_view xpub) {
    const auto payload = encoding::base58check_decode(xpub);
    if (payload.size() != 78U) throw std::invalid_argument("extended public key payload must be 78 bytes");
    ExtendedPublicKey result;
    std::copy_n(payload.begin(), 4, result.version_.begin());
    if (result.version_ != mainnet_xpub_version) throw std::invalid_argument("only mainnet xpub version is supported");
    result.depth_ = payload[4];
    std::copy_n(payload.begin() + 5, 4, result.parent_fingerprint_.begin());
    std::copy_n(payload.begin() + 9, 4, result.child_number_.begin());
    std::copy_n(payload.begin() + 13, 32, result.chain_code_.begin());
    std::copy_n(payload.begin() + 45, 33, result.public_key_.begin());
    const auto context = make_context();
    secp256k1_pubkey parsed{};
    if (secp256k1_ec_pubkey_parse(context.get(), &parsed, reinterpret_cast<const unsigned char*>(result.public_key_.data()), result.public_key_.size()) != 1) {
        throw std::invalid_argument("extended public key contains an invalid compressed public key");
    }
    return result;
}

ExtendedPublicKey ExtendedPublicKey::derive_normal(std::uint32_t index) const {
    if (index >= 0x80000000U) throw std::invalid_argument("hardened child derivation is not available from an xpub");
    std::array<std::byte, 4> serialized_index{};
    write_u32(serialized_index, index);
    std::vector<std::byte> data(public_key_.begin(), public_key_.end());
    data.insert(data.end(), serialized_index.begin(), serialized_index.end());
    const auto hmac = hashing::hmac_sha512(chain_code_, data);
    const auto context = make_context();
    secp256k1_pubkey derived{};
    if (secp256k1_ec_pubkey_parse(context.get(), &derived, reinterpret_cast<const unsigned char*>(public_key_.data()), public_key_.size()) != 1 ||
        secp256k1_ec_pubkey_tweak_add(context.get(), &derived, reinterpret_cast<const unsigned char*>(hmac.data())) != 1) {
        throw std::runtime_error("invalid BIP32 public child derivation");
    }
    ExtendedPublicKey child;
    child.version_ = version_;
    child.depth_ = std::byte{static_cast<unsigned char>(std::to_integer<unsigned char>(depth_) + 1U)};
    const auto fingerprint_hash = hashing::ripemd160(hashing::sha256(public_key_));
    std::copy_n(fingerprint_hash.begin(), 4, child.parent_fingerprint_.begin());
    child.child_number_ = serialized_index;
    std::copy_n(hmac.begin() + 32, 32, child.chain_code_.begin());
    std::size_t size = child.public_key_.size();
    if (secp256k1_ec_pubkey_serialize(context.get(), reinterpret_cast<unsigned char*>(child.public_key_.data()), &size, &derived, SECP256K1_EC_COMPRESSED) != 1 || size != child.public_key_.size()) {
        throw std::runtime_error("could not serialize BIP32 public child");
    }
    return child;
}

std::string ExtendedPublicKey::to_xpub() const {
    std::vector<std::byte> payload;
    payload.reserve(78U);
    payload.insert(payload.end(), version_.begin(), version_.end()); payload.push_back(depth_);
    payload.insert(payload.end(), parent_fingerprint_.begin(), parent_fingerprint_.end());
    payload.insert(payload.end(), child_number_.begin(), child_number_.end());
    payload.insert(payload.end(), chain_code_.begin(), chain_code_.end());
    payload.insert(payload.end(), public_key_.begin(), public_key_.end());
    return encoding::base58check_encode(payload);
}

std::vector<std::byte> ExtendedPublicKey::public_key() const { return {public_key_.begin(), public_key_.end()}; }

std::string p2pkh_address_from_xpub(const ExtendedPublicKey& xpub, std::uint32_t branch, std::uint32_t index) {
    if (branch > 1U) throw std::invalid_argument("xpub branch must be 0 or 1");
    return p2pkh_address(xpub.derive_normal(branch).derive_normal(index).public_key());
}

} // namespace crypto_research::bitcoin::bip32
