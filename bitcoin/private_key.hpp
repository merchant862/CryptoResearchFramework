#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace crypto_research::bitcoin {

class PrivateKey final {
public:
    static PrivateKey from_hex(std::string_view hex);
    static PrivateKey from_secret_scalar(std::span<const std::byte> scalar);

    [[nodiscard]] std::vector<std::byte> public_key(bool compressed = true) const;

private:
    explicit PrivateKey(std::array<std::byte, 32> bytes) noexcept;

    std::array<std::byte, 32> bytes_;
};

} // namespace crypto_research::bitcoin
