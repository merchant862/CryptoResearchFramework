#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace crypto_research::gpu {

struct ExtractionResult final {
    std::vector<std::byte> bytes;
    double kernel_milliseconds = 0.0;
};

[[nodiscard]] bool cuda_available() noexcept;

// Extracting byte bit planes is embarrassingly parallel and is the first GPU-safe
// acceleration point for image evidence processing.
[[nodiscard]] std::vector<std::byte> extract_low_bits(std::span<const std::byte> input, unsigned int bit_count);
[[nodiscard]] ExtractionResult extract_low_bits_profiled(std::span<const std::byte> input, unsigned int bit_count);

} // namespace crypto_research::gpu
