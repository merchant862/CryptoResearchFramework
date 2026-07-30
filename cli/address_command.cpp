#include "cli/address_command.hpp"

#include "bitcoin/p2pkh.hpp"
#include "bitcoin/private_key.hpp"
#include "core/utils/hex.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <string_view>

namespace crypto_research::cli {

int address_command(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        spdlog::error("Usage: crypto_research <64-character-private-key-hex> [--uncompressed]");
        return 2;
    }
    const bool compressed = argc == 2 || std::string_view{argv[2]} == "--compressed";
    if (!compressed && std::string_view{argv[2]} != "--uncompressed") {
        spdlog::error("Unknown option: {}", argv[2]);
        return 2;
    }

    try {
        const auto key = bitcoin::PrivateKey::from_hex(argv[1]);
        const auto public_key = key.public_key(compressed);
        spdlog::info("Public key: {}", utils::to_hex(public_key));
        spdlog::info("P2PKH address: {}", bitcoin::p2pkh_address(public_key));
        return 0;
    } catch (const std::exception& error) {
        spdlog::error("{}", error.what());
        return 1;
    }
}

} // namespace crypto_research::cli
