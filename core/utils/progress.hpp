#pragma once

#include <atomic>
#include <cstddef>

namespace crypto_research::utils {

class Progress final {
public:
    explicit Progress(std::size_t total) noexcept : total_(total) {}

    void complete_one() noexcept { completed_.fetch_add(1, std::memory_order_relaxed); }
    [[nodiscard]] std::size_t completed() const noexcept { return completed_.load(std::memory_order_relaxed); }
    [[nodiscard]] std::size_t total() const noexcept { return total_; }

private:
    std::size_t total_;
    std::atomic<std::size_t> completed_{};
};

} // namespace crypto_research::utils
