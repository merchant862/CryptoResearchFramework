#include "bitcoin/address/format.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

using AddressLines = std::map<std::string, std::vector<std::size_t>>;

struct AddressInput final {
    AddressLines addresses;
    std::size_t total{};
    std::size_t valid{};
    std::size_t invalid{};
};

std::string csv_escape(std::string_view value) {
    std::string out{"\""};
    for (const char c : value) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out += c;
        }
    }
    out += '"';
    return out;
}

std::string join_lines(const std::vector<std::size_t>& lines) {
    std::ostringstream out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i != 0) {
            out << ';';
        }
        out << lines[i];
    }
    return out.str();
}

void ensure_parent_directory(const char* path) {
    const std::filesystem::path output_path{path};
    if (const auto parent = output_path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

AddressInput read_addresses(const char* path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("could not open input");
    }

    AddressInput result;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        const auto format = crypto_research::bitcoin::address::inspect_public_address(line);
        if (format.normalized.empty()) {
            continue;
        }

        ++result.total;
        if (format.valid) {
            ++result.valid;
            result.addresses[format.normalized].push_back(line_number);
        } else {
            ++result.invalid;
        }
    }
    return result;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 9 || std::string_view(argv[1]) != "--left" || std::string_view(argv[3]) != "--right" ||
        std::string_view(argv[5]) != "--json-report" || std::string_view(argv[7]) != "--csv") {
        std::cerr << "Usage: address_intersection_report --left <file> --right <file> --json-report <file> --csv <file>\n";
        return 2;
    }

    try {
        const auto left = read_addresses(argv[2]);
        const auto right = read_addresses(argv[4]);

        ensure_parent_directory(argv[6]);
        ensure_parent_directory(argv[8]);

        std::ofstream csv(argv[8]);
        if (!csv) {
            throw std::runtime_error("could not open csv output");
        }

        nlohmann::json rows = nlohmann::json::array();
        csv << "address,type,left_lines,right_lines\n";

        for (const auto& [address, left_lines] : left.addresses) {
            const auto right_match = right.addresses.find(address);
            if (right_match == right.addresses.end()) {
                continue;
            }

            const auto type =
                crypto_research::bitcoin::address::to_string(crypto_research::bitcoin::address::inspect_public_address(address).type);
            rows.push_back({
                {"address", address},
                {"type", type},
                {"left_lines", left_lines},
                {"right_lines", right_match->second},
            });
            csv << csv_escape(address) << ',' << type << ',' << csv_escape(join_lines(left_lines)) << ','
                << csv_escape(join_lines(right_match->second)) << '\n';
        }

        std::ofstream out(argv[6]);
        if (!out) {
            throw std::runtime_error("could not open json output");
        }

        out << nlohmann::json{
                   {"tool", "address_intersection_report"},
                   {"summary",
                    {
                        {"total_left", left.total},
                        {"total_right", right.total},
                        {"valid_left", left.valid},
                        {"valid_right", right.valid},
                        {"invalid_left", left.invalid},
                        {"invalid_right", right.invalid},
                        {"intersection_count", rows.size()},
                    }},
                   {"matches", rows},
               }.dump(2)
            << '\n';
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
