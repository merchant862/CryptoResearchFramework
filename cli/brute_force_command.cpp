#include "cli/brute_force_command.hpp"

#include "bitcoin/address/segwit.hpp"
#include "bitcoin/bip32/xprv.hpp"
#include "bitcoin/bip39/mnemonic.hpp"
#include "bitcoin/bip39/seed.hpp"
#include "bitcoin/bip39/wordlist.hpp"
#include "bitcoin/p2pkh.hpp"

#ifdef CRYPTO_RESEARCH_WITH_CUDA
#include "gpu/brute_force_cuda.cuh"
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace crypto_research::cli {
    namespace {

        struct Options {
            std::string words_file;
            std::string wordlist_file = "wordlists/english.txt";
            std::string target_address;
            std::string path_type = "bip44";
            std::string passphrases_file;
            std::size_t workers = std::thread::hardware_concurrency();
            bool use_gpu = false;
            bool verbose = false;
        };

        Options parse_args(int argc, char* argv[]) {
            Options opts;
            for (int i = 1; i < argc; ++i) {
                std::string_view arg = argv[i];
                if (arg == "--words" && i + 1 < argc) opts.words_file = argv[++i];
                else if (arg == "--wordlist" && i + 1 < argc) opts.wordlist_file = argv[++i];
                else if (arg == "--target" && i + 1 < argc) opts.target_address = argv[++i];
                else if (arg == "--path" && i + 1 < argc) opts.path_type = argv[++i];
                else if (arg == "--passphrases" && i + 1 < argc) opts.passphrases_file = argv[++i];
                else if (arg == "--workers" && i + 1 < argc) opts.workers = std::stoul(argv[++i]);
                else if (arg == "--gpu") opts.use_gpu = true;
                else if (arg == "--verbose") opts.verbose = true;
            }
            return opts;
        }

        std::vector<std::string> load_lines(const std::string& path) {
            std::vector<std::string> lines;
            std::ifstream file(path);
            if (!file) {
                std::cerr << "Error: cannot open file: " << path << "\n";
                return lines;
            }
            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) lines.push_back(line);
            }
            return lines;
        }

        std::vector<std::string_view> build_wordlist_span(const bitcoin::bip39::EnglishWordlist& wl) {
            std::vector<std::string_view> result;
            result.reserve(wl.size());
            for (std::size_t i = 0; i < wl.size(); ++i) {
                if (auto w = wl.word_at(i)) result.push_back(*w);
            }
            return result;
        }

        std::string derive_address(const bitcoin::bip32::ExtendedPrivateKey& master,
            std::string_view path_type) {
            std::string path;
            if (path_type == "bip44") path = "m/44'/0'/0'/0/0";
            else if (path_type == "bip49") path = "m/49'/0'/0'/0/0";
            else if (path_type == "bip84") path = "m/84'/0'/0'/0/0";
            else throw std::invalid_argument("unknown path type");

            const auto child = master.derive_path(path);
            const auto pub = child.public_key();

            if (path_type == "bip44") return bitcoin::p2pkh_address(pub);
            if (path_type == "bip49") return bitcoin::address::p2sh_address(pub);
            return bitcoin::address::bech32_address(pub);
        }

        void worker_thread(std::size_t thread_id,
            std::size_t total_workers,
            const std::vector<std::string>& candidate_words,
            const std::vector<std::string>& passphrases,
            const std::vector<std::string_view>& wordlist_span,
            const Options& opts,
            std::atomic<std::uint64_t>& checked_count,
            std::atomic<std::uint64_t>& valid_count,
            std::mutex& result_mutex,
            bool& found) {
            const std::size_t n = candidate_words.size();
            if (n < 12) return;

            std::vector<std::size_t> indices(n);
            std::iota(indices.begin(), indices.end(), 0);

            std::size_t perm_count = 0;
            do {
                if (found) return;
                if (perm_count % total_workers != thread_id) {
                    ++perm_count;
                    continue;
                }
                ++perm_count;

                std::vector<std::string_view> words;
                words.reserve(12);
                for (std::size_t i = 0; i < 12; ++i) {
                    words.push_back(candidate_words[indices[i]]);
                }

                if (!bitcoin::bip39::checksum_is_valid(words, wordlist_span)) continue;
                ++valid_count;

                for (const auto& passphrase : passphrases) {
                    try {
                        const auto seed = bitcoin::bip39::mnemonic_to_seed(words, passphrase);
                        const auto master = bitcoin::bip32::ExtendedPrivateKey::from_seed(seed);
                        const auto address = derive_address(master, opts.path_type);
                        ++checked_count;

                        if (address == opts.target_address) {
                            std::lock_guard lock(result_mutex);
                            found = true;
                            std::cout << "\n*** MATCH FOUND ***\n";
                            std::cout << "Mnemonic: ";
                            for (std::size_t i = 0; i < words.size(); ++i) {
                                if (i > 0) std::cout << " ";
                                std::cout << words[i];
                            }
                            std::cout << "\n";
                            if (!passphrase.empty()) std::cout << "Passphrase: " << passphrase << "\n";
                            std::cout << "Path: " << opts.path_type << "\n";
                            std::cout << "Address: " << address << "\n";
                            std::cout << "*********************\n\n";
                            return;
                        }
                    }
                    catch (const std::exception&) {
                        // Skip invalid derivations
                    }
                }
            } while (std::next_permutation(indices.begin(), indices.end()) && !found);
        }

    } // namespace

    int brute_force_command(int argc, char* argv[]) {
        try {
            const auto opts = parse_args(argc, argv);

            if (opts.words_file.empty() || opts.target_address.empty()) {
                std::cerr << "Usage: crypto_research brute-force\n"
                    << "  --words <candidate_words.txt>\n"
                    << "  --target <bitcoin_address>\n"
                    << "  [--wordlist <english.txt>]\n"
                    << "  [--path bip44|bip49|bip84]\n"
                    << "  [--passphrases <passphrases.txt>]\n"
                    << "  [--workers <n>]\n"
                    << "  [--gpu]\n"
                    << "  [--verbose]\n";
                return 1;
            }

            const auto candidate_words = load_lines(opts.words_file);
            if (candidate_words.size() < 12) {
                std::cerr << "Error: need at least 12 candidate words, got " << candidate_words.size() << "\n";
                return 1;
            }

            auto wordlist = bitcoin::bip39::load_english_wordlist(opts.wordlist_file);
            const auto wordlist_span = build_wordlist_span(wordlist);

            std::vector<std::string> passphrases = { "" };
            if (!opts.passphrases_file.empty()) {
                const auto extra = load_lines(opts.passphrases_file);
                passphrases.insert(passphrases.end(), extra.begin(), extra.end());
            }

            std::cout << "Candidate words: " << candidate_words.size() << "\n";
            std::cout << "Target address:  " << opts.target_address << "\n";
            std::cout << "Derivation path: " << opts.path_type << "\n";
            std::cout << "Passphrases:     " << passphrases.size() << "\n";
            std::cout << "Workers:         " << opts.workers << "\n";

#ifdef CRYPTO_RESEARCH_WITH_CUDA
            if (opts.use_gpu) {
                std::cout << "Mode:            GPU (CUDA)\n";
                std::cout << "Starting GPU checksum filter...\n\n";

                auto gpu_result = gpu::brute_force_gpu(candidate_words, wordlist_span);

                if (gpu_result.total_checked == 0 && gpu_result.total_valid_checksum == 0) {
                    std::cout << "GPU filter failed or no valid words found.\n";
                    return 1;
                }

                std::cout << "GPU filter complete.\n";
                std::cout << "Total permutations checked: " << gpu_result.total_checked << "\n";
                std::cout << "Valid checksums found: " << gpu_result.valid_mnemonics.size() << "\n";
                std::cout << "GPU time: " << gpu_result.elapsed_seconds << "s\n\n";

                if (gpu_result.valid_mnemonics.empty()) {
                    std::cout << "No valid checksum mnemonics found.\n";
                    return 0;
                }

                std::cout << "Starting CPU address derivation...\n";
                std::atomic<std::uint64_t> checked_count{ 0 };
                bool found = false;
                const auto start = std::chrono::steady_clock::now();

                for (const auto& mnemonic_words : gpu_result.valid_mnemonics) {
                    if (found) break;
                    std::vector<std::string_view> words;
                    for (const auto& w : mnemonic_words) words.push_back(w);

                    for (const auto& passphrase : passphrases) {
                        try {
                            const auto seed = bitcoin::bip39::mnemonic_to_seed(words, passphrase);
                            const auto master = bitcoin::bip32::ExtendedPrivateKey::from_seed(seed);
                            const auto address = derive_address(master, opts.path_type);
                            ++checked_count;

                            if (address == opts.target_address) {
                                found = true;
                                std::cout << "\n*** MATCH FOUND ***\n";
                                std::cout << "Mnemonic: ";
                                for (std::size_t i = 0; i < words.size(); ++i) {
                                    if (i > 0) std::cout << " ";
                                    std::cout << words[i];
                                }
                                std::cout << "\n";
                                if (!passphrase.empty()) std::cout << "Passphrase: " << passphrase << "\n";
                                std::cout << "Path: " << opts.path_type << "\n";
                                std::cout << "Address: " << address << "\n";
                                std::cout << "*********************\n\n";
                                break;
                            }
                        }
                        catch (const std::exception&) {
                            // Skip
                        }
                    }
                }

                const auto elapsed = std::chrono::steady_clock::now() - start;
                const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
                std::cout << "\nBrute-force complete.\n";
                std::cout << "Total addresses checked: " << checked_count.load() << "\n";
                std::cout << "Elapsed time: " << seconds << "s\n";
                if (!found) std::cout << "No match found.\n";
                return 0;
            }
#endif

            std::cout << "Mode:            CPU\n";
            std::cout << "Starting brute-force...\n\n";

            std::atomic<std::uint64_t> checked_count{ 0 };
            std::atomic<std::uint64_t> valid_count{ 0 };
            std::mutex result_mutex;
            bool found = false;

            const auto start = std::chrono::steady_clock::now();

            std::vector<std::thread> threads;
            for (std::size_t i = 0; i < opts.workers; ++i) {
                threads.emplace_back(worker_thread, i, opts.workers,
                    std::ref(candidate_words), std::ref(passphrases),
                    std::ref(wordlist_span),
                    std::ref(opts), std::ref(checked_count),
                    std::ref(valid_count), std::ref(result_mutex),
                    std::ref(found));
            }

            std::thread reporter([&]() {
                while (!found) {
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    if (found) break;
                    const auto elapsed = std::chrono::steady_clock::now() - start;
                    const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
                    std::cout << "[Progress] Valid mnemonics checked: " << checked_count.load()
                        << " | Valid checksums: " << valid_count.load()
                        << " | Elapsed: " << seconds << "s\n";
                }
                });

            for (auto& t : threads) t.join();
            found = true;
            reporter.join();

            const auto elapsed = std::chrono::steady_clock::now() - start;
            const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();

            std::cout << "\nBrute-force complete.\n";
            std::cout << "Total valid mnemonics checked: " << checked_count.load() << "\n";
            std::cout << "Total valid checksums: " << valid_count.load() << "\n";
            std::cout << "Elapsed time: " << seconds << "s\n";

            if (!found) {
                std::cout << "No match found.\n";
            }
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
            return 1;
        }
        catch (...) {
            std::cerr << "Unknown error occurred.\n";
            return 1;
        }
    }

} // namespace crypto_research::cli