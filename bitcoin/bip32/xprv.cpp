#include "bitcoin/bip32/xprv.hpp"

#include "bitcoin/bip32/xpub.hpp"
#include "core/encoding/base58.hpp"
#include "core/hashing/hmac.hpp"
#include "core/hashing/ripemd160.hpp"
#include "core/hashing/sha256.hpp"

#include <secp256k1.h>

#include <algorithm>
#include <charconv>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace crypto_research::bitcoin::bip32 {
    namespace {

        constexpr std::array<std::byte, 4> mainnet_xprv_version{
            std::byte{0x04}, std::byte{0x88}, std::byte{0xad}, std::byte{0xe4}
        };

        struct ContextDeleter {
            void operator()(secp256k1_context* context) const noexcept {
                secp256k1_context_destroy(context);
            }
        };
        using Context = std::unique_ptr<secp256k1_context, ContextDeleter>;

        Context make_context() {
            Context context{ secp256k1_context_create(SECP256K1_CONTEXT_SIGN) };
            if (!context) throw std::runtime_error("could not create a libsecp256k1 context");
            return context;
        }

    } // namespace

    ExtendedPrivateKey ExtendedPrivateKey::from_seed(std::span<const std::byte> seed) {
        constexpr std::string_view seed_key = "Bitcoin seed";
        const auto seed_key_bytes = std::as_bytes(std::span{ seed_key });
        const auto hmac = hashing::hmac_sha512(seed_key_bytes, seed);

        ExtendedPrivateKey master;
        std::copy_n(hmac.begin() + 32, 32, master.chain_code_.begin());
        std::copy_n(hmac.begin(), 32, master.private_key_.begin());

        // Validate private key is non-zero and less than curve order.
        const auto context = make_context();
        if (secp256k1_ec_seckey_verify(context.get(),
            reinterpret_cast<const unsigned char*>(master.private_key_.data())) != 1) {
            throw std::runtime_error("invalid BIP32 master private key");
        }
        return master;
    }

    ExtendedPrivateKey ExtendedPrivateKey::derive_hardened(std::uint32_t index) const {
        if (index < 0x80000000U) {
            throw std::invalid_argument("hardened index must be >= 0x80000000");
        }
        return derive_normal(index);
    }

    ExtendedPrivateKey ExtendedPrivateKey::derive_normal(std::uint32_t index) const {
        std::array<std::byte, 4> serialized_index{};
        write_u32(serialized_index, index);

        std::vector<std::byte> data;
        if (index >= 0x80000000U) {
            // Hardened: 0x00 || private_key || index
            data.reserve(1 + 32 + 4);
            data.push_back(std::byte{ 0x00 });
            data.insert(data.end(), private_key_.begin(), private_key_.end());
        }
        else {
            // Non-hardened: public_key(compressed) || index
            data = public_key();
        }
        data.insert(data.end(), serialized_index.begin(), serialized_index.end());

        const auto hmac = hashing::hmac_sha512(
            std::span{ chain_code_ }, std::span{ data });

        const auto context = make_context();

        // Child private key = parent_private_key + tweak (mod n)
        ExtendedPrivateKey child;
        child.version_ = version_;
        child.depth_ = std::byte{ static_cast<unsigned char>(std::to_integer<unsigned char>(depth_) + 1U) };
        child.child_number_ = serialized_index;

        const auto fingerprint_hash = hashing::ripemd160(hashing::sha256(public_key()));
        std::copy_n(fingerprint_hash.begin(), 4, child.parent_fingerprint_.begin());

        std::copy_n(hmac.begin() + 32, 32, child.chain_code_.begin());
        std::copy_n(private_key_.begin(), 32, child.private_key_.begin());

        if (secp256k1_ec_seckey_tweak_add(context.get(),
            reinterpret_cast<unsigned char*>(child.private_key_.data()),
            reinterpret_cast<const unsigned char*>(hmac.data())) != 1) {
            throw std::runtime_error("invalid BIP32 private child derivation");
        }

        if (secp256k1_ec_seckey_verify(context.get(),
            reinterpret_cast<const unsigned char*>(child.private_key_.data())) != 1) {
            throw std::runtime_error("invalid BIP32 derived private key");
        }

        return child;
    }

    ExtendedPrivateKey ExtendedPrivateKey::derive_path(std::string_view path) const {
        if (path.empty() || path[0] != 'm') {
            throw std::invalid_argument("BIP32 path must start with 'm'");
        }
        if (std::to_integer<unsigned char>(depth_) != 0U) {
            throw std::invalid_argument("derive_path must be called on a master key");
        }

        ExtendedPrivateKey current = *this;
        std::size_t pos = 1; // skip 'm'
        while (pos < path.size()) {
            if (path[pos] == '/') ++pos;
            if (pos >= path.size()) break;

            std::size_t end = pos;
            while (end < path.size() && path[end] != '/') ++end;

            std::string_view segment = path.substr(pos, end - pos);
            bool hardened = false;
            if (!segment.empty() && (segment.back() == '\'' || segment.back() == 'h' || segment.back() == 'H')) {
                hardened = true;
                segment = segment.substr(0, segment.size() - 1);
            }

            std::uint32_t index = 0;
            auto [ptr, ec] = std::from_chars(segment.data(), segment.data() + segment.size(), index);
            if (ec != std::errc{}) {
                throw std::invalid_argument("invalid BIP32 path segment");
            }
            if (hardened) index |= 0x80000000U;

            current = current.derive_normal(index);
            pos = end;
        }
        return current;
    }

    std::vector<std::byte> ExtendedPrivateKey::private_key() const {
        return { private_key_.begin(), private_key_.end() };
    }

    std::vector<std::byte> ExtendedPrivateKey::public_key() const {
        const auto context = make_context();
        secp256k1_pubkey pubkey{};
        if (secp256k1_ec_pubkey_create(context.get(), &pubkey,
            reinterpret_cast<const unsigned char*>(private_key_.data())) != 1) {
            throw std::runtime_error("could not create public key from private key");
        }
        std::array<std::byte, 33> compressed{};
        std::size_t size = compressed.size();
        if (secp256k1_ec_pubkey_serialize(context.get(),
            reinterpret_cast<unsigned char*>(compressed.data()), &size, &pubkey,
            SECP256K1_EC_COMPRESSED) != 1 || size != compressed.size()) {
            throw std::runtime_error("could not serialize public key");
        }
        return { compressed.begin(), compressed.end() };
    }

    std::vector<std::byte> ExtendedPrivateKey::chain_code() const {
        return { chain_code_.begin(), chain_code_.end() };
    }

    std::string ExtendedPrivateKey::to_xprv() const {
        std::vector<std::byte> payload;
        payload.reserve(78U);
        payload.insert(payload.end(), version_.begin(), version_.end());
        payload.push_back(depth_);
        payload.insert(payload.end(), parent_fingerprint_.begin(), parent_fingerprint_.end());
        payload.insert(payload.end(), child_number_.begin(), child_number_.end());
        payload.insert(payload.end(), chain_code_.begin(), chain_code_.end());
        payload.push_back(std::byte{ 0x00 });
        payload.insert(payload.end(), private_key_.begin(), private_key_.end());
        return encoding::base58check_encode(payload);
    }

    ExtendedPublicKey ExtendedPrivateKey::to_xpub() const {
        // Reconstruct xpub from xprv components (public key derived, private key discarded)
        const auto pub = public_key();
        if (pub.size() != 33U) throw std::runtime_error("invalid compressed public key size");

        std::vector<std::byte> payload;
        payload.reserve(78U);
        // xpub version
        payload.push_back(std::byte{ 0x04 }); payload.push_back(std::byte{ 0x88 });
        payload.push_back(std::byte{ 0xb2 }); payload.push_back(std::byte{ 0x1e });
        payload.push_back(depth_);
        payload.insert(payload.end(), parent_fingerprint_.begin(), parent_fingerprint_.end());
        payload.insert(payload.end(), child_number_.begin(), child_number_.end());
        payload.insert(payload.end(), chain_code_.begin(), chain_code_.end());
        payload.insert(payload.end(), pub.begin(), pub.end());
        return ExtendedPublicKey::parse(encoding::base58check_encode(payload));
    }

    std::array<std::byte, 4> ExtendedPrivateKey::fingerprint() const {
        const auto hash = hashing::ripemd160(hashing::sha256(public_key()));
        std::array<std::byte, 4> result{};
        std::copy_n(hash.begin(), 4, result.begin());
        return result;
    }

    void ExtendedPrivateKey::write_u32(std::array<std::byte, 4>& output, std::uint32_t value) const {
        for (int position = 3; position >= 0; --position) {
            output[static_cast<std::size_t>(position)] = std::byte{ static_cast<unsigned char>(value & 0xffU) };
            value >>= 8U;
        }
    }

} // namespace crypto_research::bitcoin::bip32