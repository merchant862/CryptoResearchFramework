#include "bitcoin/address/public_address_matcher.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace crypto_research::bitcoin::address {

std::vector<PublicAddressMatch> compare_public_address_file(const std::filesystem::path& addresses_file,
                                                             std::string_view target_address) {
    std::ifstream input(addresses_file);
    if (!input) throw std::runtime_error("could not open public address file: " + addresses_file.string());
    std::vector<PublicAddressMatch> results;
    std::string address;
    std::size_t line_number = 0;
    while (std::getline(input, address)) {
        ++line_number;
        if (!address.empty() && address.back() == '\r') address.pop_back();
        if (!address.empty()) results.push_back({line_number, address == target_address, std::move(address)});
    }
    return results;
}

} // namespace crypto_research::bitcoin::address
