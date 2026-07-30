#include "bitcoin/address/format.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

namespace {

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

void ensure_parent_directory(const char* path) {
    const std::filesystem::path output_path{path};
    if (const auto parent = output_path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 7 || std::string_view(argv[1]) != "--input" || std::string_view(argv[3]) != "--json-report" ||
        std::string_view(argv[5]) != "--csv") {
        std::cerr << "Usage: address_format_report --input <file> --json-report <file> --csv <file>\n";
        return 2;
    }

    std::ifstream in(argv[2]);
    if (!in) {
        std::cerr << "could not open input\n";
        return 1;
    }

    ensure_parent_directory(argv[4]);
    ensure_parent_directory(argv[6]);

    std::ofstream csv(argv[6]);
    if (!csv) {
        std::cerr << "could not open csv output\n";
        return 1;
    }

    nlohmann::json rows = nlohmann::json::array();
    csv << "line_number,original,normalized,type,valid,error\n";

    std::string line;
    std::size_t line_number = 0;
    while (std::getline(in, line)) {
        ++line_number;
        const auto format = crypto_research::bitcoin::address::inspect_public_address(line);
        if (format.normalized.empty()) {
            continue;
        }

        const auto type = crypto_research::bitcoin::address::to_string(format.type);
        rows.push_back({
            {"line_number", line_number},
            {"original", line},
            {"normalized", format.normalized},
            {"type", type},
            {"valid", format.valid},
            {"error", format.error},
        });
        csv << line_number << ',' << csv_escape(line) << ',' << csv_escape(format.normalized) << ',' << type << ','
            << (format.valid ? "true" : "false") << ',' << csv_escape(format.error) << '\n';
    }

    std::ofstream out(argv[4]);
    if (!out) {
        std::cerr << "could not open json output\n";
        return 1;
    }

    out << nlohmann::json{{"tool", "address_format_report"}, {"rows", rows}}.dump(2) << '\n';
}
