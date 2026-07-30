#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace crypto_research::wordlists {

class LocalWordlist final {
public:
    static LocalWordlist load(const std::filesystem::path& path, std::size_t expected_count);

    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] std::optional<std::size_t> index_of(std::string_view word) const;
    [[nodiscard]] std::optional<std::string_view> word_at(std::size_t index) const;

private:
    explicit LocalWordlist(std::vector<std::string> words);

    std::vector<std::string> words_;
    std::unordered_map<std::string, std::size_t> indexes_;
};

} // namespace crypto_research::wordlists
