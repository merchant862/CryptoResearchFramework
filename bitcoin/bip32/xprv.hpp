#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace crypto_research::bitcoin::bip32 {

class ExtendedPublicKey;

// BIP32 extended private key (xprv) supporting hardened and non-hardened
// child derivation, compatible with BIP44 / BIP49 / BIP84 paths.
class ExtendedPrivateKey {
public:
    // Derive master extended private key from a BIP39 seed (64 bytes).
    [[nodiscard]] static ExtendedPrivateKey from_seed(std::span<const std::byte> seed);

    // Hardened child derivation (e.g. 44', 0', 0').
    [[nodiscard]] ExtendedPrivateKey derive_hardened(std::uint32_t index) const;

    // Non-hardened child derivation (e.g. 0, 0).
    [[nodiscard]] ExtendedPrivateKey derive_normal(std::uint32_t index) const;

    // Derive a full path like m/44'/0'/0'/0/0 from this key (must be master).
    [[nodiscard]] ExtendedPrivateKey derive_path(std::string_view path) const;

    [[nodiscard]] std::vector<std::byte> private_key() const;
    [[nodiscard]] std::vector<std::byte> public_key() const;
    [[nodiscard]] std::vector<std::byte> chain_code() const;
    [[nodiscard]] std::string to_xprv() const;

    // Convert to xpub (public-only extended key).
    [[nodiscard]] ExtendedPublicKey to_xpub() const;

    [[nodiscard]] std::byte depth() const noexcept { return depth_; }

private:
    std::array<std::byte, 4> version_{std::byte{0x04}, std::byte{0x88}, std::byte{0xad}, std::byte{0xe4}}; // mainnet xprv
    std::byte depth_{std::byte{0x00}};
    std::array<std::byte, 4> parent_fingerprint_{};
    std::array<std::byte, 4> child_number_{};
    std::array<std::byte, 32> chain_code_{};
    std::array<std::byte, 32> private_key_{};

    ExtendedPrivateKey() = default;

    [[nodiscard]] std::array<std::byte, 4> fingerprint() const;
    void write_u32(std::array<std::byte, 4>& output, std::uint32_t value) const;
};

} // namespace crypto_research::bitcoin::bip32
