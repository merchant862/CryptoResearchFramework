#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace crypto_research::bitcoin::bip32 {

class ExtendedPublicKey final {
public:
    static ExtendedPublicKey parse(std::string_view xpub);

    [[nodiscard]] ExtendedPublicKey derive_normal(std::uint32_t index) const;
    [[nodiscard]] std::string to_xpub() const;
    [[nodiscard]] std::vector<std::byte> public_key() const;

private:
    std::array<std::byte, 4> version_{};
    std::byte depth_{};
    std::array<std::byte, 4> parent_fingerprint_{};
    std::array<std::byte, 4> child_number_{};
    std::array<std::byte, 32> chain_code_{};
    std::array<std::byte, 33> public_key_{};

    friend std::string p2pkh_address_from_xpub(const ExtendedPublicKey&, std::uint32_t, std::uint32_t);
};

[[nodiscard]] std::string p2pkh_address_from_xpub(const ExtendedPublicKey& xpub,
                                                   std::uint32_t branch,
                                                   std::uint32_t index);

} // namespace crypto_research::bitcoin::bip32
