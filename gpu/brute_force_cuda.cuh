#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace crypto_research::gpu {

    struct BruteForceResult {
        std::vector<std::vector<std::string>> valid_mnemonics;
        std::uint64_t total_checked = 0;
        std::uint64_t total_valid_checksum = 0;
        double elapsed_seconds = 0.0;
    };

    // GPU-accelerated BIP39 checksum validation + permutation generation.
    // Returns all valid-checksum mnemonics from the candidate pool.
    // The caller must then derive addresses on CPU.
    BruteForceResult brute_force_gpu(
        const std::vector<std::string>& candidate_words,
        const std::vector<std::string_view>& wordlist,
        std::size_t max_results = 1000000);

} // namespace crypto_research::gpu