#include "bitcoin/address/public_address_matcher.hpp"

#include <exception>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc != 5 || std::string_view{argv[1]} != "--addresses" || std::string_view{argv[3]} != "--target") {
        std::cerr << "Usage: public_address_list_matcher --addresses <addresses.txt> --target <public-address>\n";
        return 2;
    }
    try {
        for (const auto& result : crypto_research::bitcoin::address::compare_public_address_file(argv[2], argv[4])) {
            std::cout << result.line_number << ' ' << (result.matches ? "MATCH" : "NO MATCH")
                      << ' ' << result.address << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
