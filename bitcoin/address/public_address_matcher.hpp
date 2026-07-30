#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace crypto_research::bitcoin::address {

struct PublicAddressMatch final {
    std::size_t line_number{};
    bool matches{};
    std::string address;
};

[[nodiscard]] std::vector<PublicAddressMatch> compare_public_address_file(const std::filesystem::path& addresses_file,
                                                                           std::string_view target_address);

} // namespace crypto_research::bitcoin::address
