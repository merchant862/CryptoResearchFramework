#include "cli/address_command.hpp"
#include "cli/batch_image_command.hpp"
#include "cli/image_command.hpp"
#include "cli/brute_force_command.hpp"

#include <string_view>

int main(int argc, char* argv[]) {
    if (argc > 1 && std::string_view{ argv[1] } == "analyze-image") return crypto_research::cli::image_command(argc, argv);
    if (argc > 1 && std::string_view{ argv[1] } == "batch-analyze") return crypto_research::cli::batch_image_command(argc, argv);
    if (argc > 1 && std::string_view{ argv[1] } == "address") return crypto_research::cli::address_command(argc - 1, argv + 1);
    if (argc > 1 && std::string_view{ argv[1] } == "brute-force") return crypto_research::cli::brute_force_command(argc - 1, argv + 1);
    return crypto_research::cli::address_command(argc, argv);
}