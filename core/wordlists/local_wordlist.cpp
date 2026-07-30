#include "core/wordlists/local_wordlist.hpp"

#include <fstream>
#include <stdexcept>

namespace crypto_research::wordlists {
namespace {

void trim_cr(std::string& word) {
    if (!word.empty() && word.back() == '\r') word.pop_back();
}

} // namespace

LocalWordlist::LocalWordlist(std::vector<std::string> words) : words_(std::move(words)) {
    for (std::size_t index = 0; index < words_.size(); ++index) {
        if (!indexes_.emplace(words_[index], index).second) {
            throw std::invalid_argument("word list contains a duplicate word: " + words_[index]);
        }
    }
}

LocalWordlist LocalWordlist::load(const std::filesystem::path& path, std::size_t expected_count) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("could not open word list: " + path.string());
    std::vector<std::string> words;
    std::string word;
    while (std::getline(input, word)) {
        trim_cr(word);
        if (words.empty() && word.starts_with("\xEF\xBB\xBF")) word.erase(0, 3);
        if (word.empty()) throw std::invalid_argument("word list contains an empty line");
        words.push_back(std::move(word));
    }
    if (words.size() != expected_count) {
        throw std::invalid_argument("word list must contain exactly " + std::to_string(expected_count) + " words");
    }
    return LocalWordlist{std::move(words)};
}

std::size_t LocalWordlist::size() const noexcept { return words_.size(); }

std::optional<std::size_t> LocalWordlist::index_of(std::string_view word) const {
    const auto iterator = indexes_.find(std::string{word});
    if (iterator == indexes_.end()) return std::nullopt;
    return iterator->second;
}

std::optional<std::string_view> LocalWordlist::word_at(std::size_t index) const {
    if (index >= words_.size()) return std::nullopt;
    return words_[index];
}

} // namespace crypto_research::wordlists
