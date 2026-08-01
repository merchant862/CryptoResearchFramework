#include "gpu/brute_force_cuda.cuh"

#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace crypto_research::gpu {
    namespace {

        // ------------------------------------------------------------------
        // Device SHA256 (minimal implementation for BIP39 checksum)
        // ------------------------------------------------------------------
        __constant__ std::uint32_t d_k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };

        __device__ std::uint32_t d_rotr(std::uint32_t x, std::uint32_t n) {
            return (x >> n) | (x << (32 - n));
        }

        __device__ void d_sha256_transform(std::uint32_t* state, const std::uint8_t* block) {
            std::uint32_t w[64];
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<std::uint32_t>(block[i * 4]) << 24) |
                    (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                    (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                    (static_cast<std::uint32_t>(block[i * 4 + 3]));
            }
            for (int i = 16; i < 64; ++i) {
                std::uint32_t s0 = d_rotr(w[i - 15], 7) ^ d_rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                std::uint32_t s1 = d_rotr(w[i - 2], 17) ^ d_rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                w[i] = w[i - 16] + s0 + w[i - 7] + s1;
            }

            std::uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
            std::uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

            for (int i = 0; i < 64; ++i) {
                std::uint32_t S1 = d_rotr(e, 6) ^ d_rotr(e, 11) ^ d_rotr(e, 25);
                std::uint32_t ch = (e & f) ^ ((~e) & g);
                std::uint32_t temp1 = h + S1 + ch + d_k[i] + w[i];
                std::uint32_t S0 = d_rotr(a, 2) ^ d_rotr(a, 13) ^ d_rotr(a, 22);
                std::uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
                std::uint32_t temp2 = S0 + maj;
                h = g; g = f; f = e; e = d + temp1; d = c; c = b; b = a; a = temp1 + temp2;
            }

            state[0] += a; state[1] += b; state[2] += c; state[3] += d;
            state[4] += e; state[5] += f; state[6] += g; state[7] += h;
        }

        __device__ void d_sha256(const std::uint8_t* data, std::size_t len, std::uint8_t* hash) {
            std::uint32_t state[8] = {
                0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
            };

            std::size_t full_blocks = len / 64;
            for (std::size_t i = 0; i < full_blocks; ++i) {
                d_sha256_transform(state, data + i * 64);
            }

            std::uint8_t last_block[64] = {};
            std::size_t remainder = len % 64;
            for (std::size_t i = 0; i < remainder; ++i) last_block[i] = data[full_blocks * 64 + i];
            last_block[remainder] = 0x80;
            std::uint64_t bit_len = static_cast<std::uint64_t>(len) * 8;
            if (remainder >= 56) {
                d_sha256_transform(state, last_block);
                std::uint8_t pad[64] = {};
                for (int i = 0; i < 8; ++i) pad[56 + i] = static_cast<std::uint8_t>(bit_len >> (56 - i * 8));
                d_sha256_transform(state, pad);
            }
            else {
                for (int i = 0; i < 8; ++i) last_block[56 + i] = static_cast<std::uint8_t>(bit_len >> (56 - i * 8));
                d_sha256_transform(state, last_block);
            }

            for (int i = 0; i < 8; ++i) {
                hash[i * 4] = static_cast<std::uint8_t>(state[i] >> 24);
                hash[i * 4 + 1] = static_cast<std::uint8_t>(state[i] >> 16);
                hash[i * 4 + 2] = static_cast<std::uint8_t>(state[i] >> 8);
                hash[i * 4 + 3] = static_cast<std::uint8_t>(state[i]);
            }
        }

        // ------------------------------------------------------------------
        // Device BIP39 Checksum Validation
        // ------------------------------------------------------------------
        __device__ bool d_bip39_checksum_valid(const std::uint16_t* word_indices, int word_count) {
            const int checksum_bits = word_count / 3;
            const int entropy_bits = word_count * 11 - checksum_bits;
            const int entropy_bytes = entropy_bits / 8;

            std::uint8_t entropy[32] = {};
            for (int i = 0; i < entropy_bits; ++i) {
                int word_idx = i / 11;
                int bit_in_word = 10 - (i % 11);
                if ((word_indices[word_idx] >> bit_in_word) & 1) {
                    entropy[i / 8] |= (1 << (7 - (i % 8)));
                }
            }

            std::uint8_t digest[32];
            d_sha256(entropy, entropy_bytes, digest);

            for (int i = 0; i < checksum_bits; ++i) {
                int expected = (digest[i / 8] >> (7 - (i % 8))) & 1;
                int word_idx = (entropy_bits + i) / 11;
                int bit_in_word = 10 - ((entropy_bits + i) % 11);
                int actual = (word_indices[word_idx] >> bit_in_word) & 1;
                if (expected != actual) return false;
            }
            return true;
        }

        // ------------------------------------------------------------------
        // Factorial Number System Permutation
        // ------------------------------------------------------------------
        __device__ void d_get_permutation(std::uint64_t perm_idx, int n, int k, std::uint16_t* out) {
            bool used[20] = { false };
            std::uint64_t fact[20];
            fact[0] = 1;
            for (int i = 1; i < n; ++i) fact[i] = fact[i - 1] * i;

            std::uint64_t remaining = perm_idx;
            for (int i = 0; i < k; ++i) {
                std::uint64_t f = fact[n - 1 - i];
                int idx = (f == 0) ? 0 : static_cast<int>(remaining / f);
                if (idx >= n - i) idx = n - i - 1;
                remaining = (f == 0) ? remaining : remaining % f;

                int count = 0;
                for (int j = 0; j < n; ++j) {
                    if (!used[j]) {
                        if (count == idx) {
                            out[i] = j;
                            used[j] = true;
                            break;
                        }
                        ++count;
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Kernel
        // ------------------------------------------------------------------
        __global__ void brute_force_kernel(
            const std::uint16_t* d_candidate_wordlist_indices,
            int num_candidates,
            std::uint64_t start_perm,
            std::uint64_t end_perm,
            std::uint16_t* d_valid_mnemonics,
            int* d_valid_count,
            int max_results,
            std::uint64_t* d_checked_count)
        {
            std::uint64_t tid = blockIdx.x * blockDim.x + threadIdx.x;
            std::uint64_t total_threads = gridDim.x * blockDim.x;

            std::uint64_t range = end_perm - start_perm;
            std::uint64_t per_thread = (range + total_threads - 1) / total_threads;
            std::uint64_t my_start = start_perm + tid * per_thread;
            std::uint64_t my_end = (my_start + per_thread > end_perm) ? end_perm : my_start + per_thread;

            std::uint16_t perm[12];
            std::uint16_t mnemonic_indices[12];

            for (std::uint64_t p = my_start; p < my_end; ++p) {
                d_get_permutation(p, num_candidates, 12, perm);
                for (int i = 0; i < 12; ++i) {
                    mnemonic_indices[i] = d_candidate_wordlist_indices[perm[i]];
                }

                atomicAdd(reinterpret_cast<unsigned long long*>(d_checked_count), 1ULL);

                if (d_bip39_checksum_valid(mnemonic_indices, 12)) {
                    int idx = atomicAdd(d_valid_count, 1);
                    if (idx < max_results) {
                        for (int i = 0; i < 12; ++i) {
                            d_valid_mnemonics[idx * 12 + i] = perm[i];
                        }
                    }
                }
            }
        }

        // ------------------------------------------------------------------
        // Helper: Build candidate wordlist indices (returns false if any word invalid)
        // ------------------------------------------------------------------
        bool build_candidate_indices(
            const std::vector<std::string>& candidate_words,
            const std::vector<std::string_view>& wordlist,
            std::vector<std::uint16_t>& out_indices,
            std::vector<std::string>& out_valid_words)
        {
            out_indices.clear();
            out_valid_words.clear();
            for (const auto& cw : candidate_words) {
                bool found = false;
                for (std::size_t i = 0; i < wordlist.size(); ++i) {
                    if (wordlist[i] == cw) {
                        out_indices.push_back(static_cast<std::uint16_t>(i));
                        out_valid_words.push_back(cw);
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cerr << "[WARNING] Candidate word not in BIP39 wordlist, skipping: " << cw << "\n";
                }
            }
            return out_indices.size() >= 12;
        }

        std::uint64_t factorial(int n) {
            std::uint64_t result = 1;
            for (int i = 2; i <= n; ++i) result *= i;
            return result;
        }

    } // namespace

    // ------------------------------------------------------------------
    // Public API
    // ------------------------------------------------------------------
    BruteForceResult brute_force_gpu(
        const std::vector<std::string>& candidate_words,
        const std::vector<std::string_view>& wordlist,
        std::size_t max_results)
    {
        BruteForceResult result;

        std::vector<std::uint16_t> candidate_indices;
        std::vector<std::string> valid_candidate_words;
        if (!build_candidate_indices(candidate_words, wordlist, candidate_indices, valid_candidate_words)) {
            std::cerr << "[ERROR] Need at least 12 valid candidate words in BIP39 wordlist.\n";
            result.total_checked = 0;
            result.total_valid_checksum = 0;
            result.elapsed_seconds = 0;
            return result;
        }

        int num_candidates = static_cast<int>(candidate_indices.size());
        if (num_candidates > 20) {
            std::cerr << "[ERROR] GPU brute-force supports max 20 candidate words.\n";
            result.total_checked = 0;
            result.total_valid_checksum = 0;
            result.elapsed_seconds = 0;
            return result;
        }

        std::cout << "[GPU] Valid candidate words: " << num_candidates << " / " << candidate_words.size() << "\n";

        const auto start_time = std::chrono::steady_clock::now();
        std::uint64_t total_perms = factorial(num_candidates);

        // Device allocations
        std::uint16_t* d_candidate_indices = nullptr;
        std::uint16_t* d_valid_mnemonics = nullptr;
        int* d_valid_count = nullptr;
        std::uint64_t* d_checked_count = nullptr;

        cudaMalloc(&d_candidate_indices, candidate_indices.size() * sizeof(std::uint16_t));
        cudaMalloc(&d_valid_mnemonics, max_results * 12 * sizeof(std::uint16_t));
        cudaMalloc(&d_valid_count, sizeof(int));
        cudaMalloc(&d_checked_count, sizeof(std::uint64_t));

        cudaMemcpy(d_candidate_indices, candidate_indices.data(),
            candidate_indices.size() * sizeof(std::uint16_t), cudaMemcpyHostToDevice);
        cudaMemset(d_valid_count, 0, sizeof(int));
        cudaMemset(d_checked_count, 0, sizeof(std::uint64_t));

        // Launch config: RTX 5060 ~3840 CUDA cores
        const int block_size = 256;
        const int num_blocks = 3840;

        brute_force_kernel << <num_blocks, block_size >> > (
            d_candidate_indices, num_candidates,
            0, total_perms,
            d_valid_mnemonics, d_valid_count,
            static_cast<int>(max_results),
            d_checked_count);

        cudaDeviceSynchronize();

        // Copy results back
        int h_valid_count = 0;
        std::uint64_t h_checked_count = 0;
        cudaMemcpy(&h_valid_count, d_valid_count, sizeof(int), cudaMemcpyDeviceToHost);
        cudaMemcpy(&h_checked_count, d_checked_count, sizeof(std::uint64_t), cudaMemcpyDeviceToHost);

        std::vector<std::uint16_t> h_valid_mnemonics(max_results * 12);
        cudaMemcpy(h_valid_mnemonics.data(), d_valid_mnemonics,
            max_results * 12 * sizeof(std::uint16_t), cudaMemcpyDeviceToHost);

        // Cleanup
        cudaFree(d_candidate_indices);
        cudaFree(d_valid_mnemonics);
        cudaFree(d_valid_count);
        cudaFree(d_checked_count);

        result.total_checked = h_checked_count;
        result.total_valid_checksum = static_cast<std::uint64_t>(h_valid_count);

        int count = (h_valid_count < static_cast<int>(max_results)) ? h_valid_count : static_cast<int>(max_results);
        for (int i = 0; i < count; ++i) {
            std::vector<std::string> mnemonic;
            for (int j = 0; j < 12; ++j) {
                int perm_idx = h_valid_mnemonics[i * 12 + j];
                mnemonic.push_back(valid_candidate_words[perm_idx]);
            }
            result.valid_mnemonics.push_back(std::move(mnemonic));
        }

        const auto end_time = std::chrono::steady_clock::now();
        result.elapsed_seconds = std::chrono::duration<double>(end_time - start_time).count();

        return result;
    }

} // namespace crypto_research::gpu