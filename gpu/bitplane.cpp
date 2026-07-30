#include "gpu/bitplane.hpp"

#include <stdexcept>

namespace crypto_research::gpu {

bool cuda_available() noexcept { return false; }

std::vector<std::byte> extract_low_bits(std::span<const std::byte>, unsigned int) {
    throw std::runtime_error("CUDA support was not enabled; configure with CRYPTO_RESEARCH_ENABLE_CUDA=ON");
}

ExtractionResult extract_low_bits_profiled(std::span<const std::byte> input, unsigned int bit_count) {
    return {.bytes = extract_low_bits(input, bit_count)};
}

} // namespace crypto_research::gpu
