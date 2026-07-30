#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct TermStats final {
    std::size_t count{};
    std::set<std::size_t> lines;
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

void ensure_parent_directory(const char* path) {
    const std::filesystem::path output_path{path};
    if (const auto parent = output_path.parent_path(); !parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

std::string normalize_token(std::string_view raw) {
    std::string token;
    token.reserve(raw.size());
    for (const char c : raw) {
        token.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return token;
}

std::vector<std::string> tokenize(std::string_view line) {
    std::vector<std::string> tokens;
    std::string current;

    for (const char c : line) {
        const auto ch = static_cast<unsigned char>(c);
        if (std::isalnum(ch)) {
            current.push_back(static_cast<char>(ch));
            continue;
        }
        if (!current.empty()) {
            tokens.push_back(normalize_token(current));
            current.clear();
        }
    }

    if (!current.empty()) {
        tokens.push_back(normalize_token(current));
    }
    return tokens;
}

std::string join_lines(const std::set<std::size_t>& lines) {
    std::ostringstream out;
    bool first = true;
    for (const auto line : lines) {
        if (!first) {
            out << ';';
        }
        first = false;
        out << line;
    }
    return out.str();
}

std::string classify_token(std::string_view token) {
    const bool has_alpha = std::any_of(token.begin(), token.end(), [](char c) {
        return std::isalpha(static_cast<unsigned char>(c)) != 0;
    });
    const bool has_digit = std::any_of(token.begin(), token.end(), [](char c) {
        return std::isdigit(static_cast<unsigned char>(c)) != 0;
    });

    if (has_alpha && has_digit) {
        return "alphanumeric";
    }
    if (has_alpha) {
        return "word";
    }
    if (has_digit) {
        return "number";
    }
    return "other";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 7 || std::string_view(argv[1]) != "--input" || std::string_view(argv[3]) != "--json-report" ||
        std::string_view(argv[5]) != "--csv") {
        std::cerr << "Usage: evidence_text_report --input <file> --json-report <file> --csv <file>\n";
        return 2;
    }

    try {
        std::ifstream input(argv[2]);
        if (!input) {
            throw std::runtime_error("could not open input");
        }

        std::map<std::string, TermStats> terms;
        std::size_t total_lines = 0;
        std::size_t total_tokens = 0;

        std::string line;
        while (std::getline(input, line)) {
            ++total_lines;
            for (const auto& token : tokenize(line)) {
                if (token.empty()) {
                    continue;
                }
                ++total_tokens;
                auto& stats = terms[token];
                ++stats.count;
                stats.lines.insert(total_lines);
            }
        }

        ensure_parent_directory(argv[3]);
        ensure_parent_directory(argv[5]);

        std::ofstream csv(argv[5]);
        if (!csv) {
            throw std::runtime_error("could not open csv output");
        }
        csv << "term,count,type,line_count,lines\n";

        nlohmann::json rows = nlohmann::json::array();
        for (const auto& [term, stats] : terms) {
            const auto type = classify_token(term);
            const auto lines = join_lines(stats.lines);
            rows.push_back({
                {"term", term},
                {"count", stats.count},
                {"type", type},
                {"line_count", stats.lines.size()},
                {"lines", stats.lines},
            });
            csv << csv_escape(term) << ',' << stats.count << ',' << type << ',' << stats.lines.size() << ','
                << csv_escape(lines) << '\n';
        }

        std::ofstream json_output(argv[3]);
        if (!json_output) {
            throw std::runtime_error("could not open json output");
        }

        json_output << nlohmann::json{
                           {"tool", "evidence_text_report"},
                           {"input", argv[2]},
                           {"summary",
                            {
                                {"total_lines", total_lines},
                                {"total_tokens", total_tokens},
                                {"unique_terms", terms.size()},
                            }},
                           {"terms", rows},
                       }.dump(2)
                    << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
