#include "bitcoin/p2pkh.hpp"
#include "bitcoin/private_key.hpp"
#include "bitcoin/bip39/mnemonic.hpp"
#include "core/encoding/base58.hpp"
#include "core/hashing/ripemd160.hpp"
#include "core/hashing/sha256.hpp"
#include "core/utils/hex.hpp"
#include "steganography/bitplane.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <span>
#include <string_view>
#include <vector>

namespace {

template <typename Work>
void benchmark(std::string_view name, std::size_t iterations, Work&& work) {
    const auto started = std::chrono::steady_clock::now();
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) work();
    const auto elapsed = std::chrono::duration<double>{std::chrono::steady_clock::now() - started}.count();
    std::cout << name << ": " << std::fixed << std::setprecision(0)
              << static_cast<double>(iterations) / elapsed << " operations/s\n";
}

} // namespace

int main() {
    constexpr std::string_view message = "offline public cryptography benchmark";
    const auto bytes = std::as_bytes(std::span{message.data(), message.size()});
    const auto test_scalar = crypto_research::utils::from_hex("0000000000000000000000000000000000000000000000000000000000000001");
    const auto public_key = crypto_research::bitcoin::PrivateKey::from_secret_scalar(test_scalar).public_key();
    const auto p2pkh = crypto_research::bitcoin::p2pkh_address(public_key);
    std::vector<std::byte> image(690U * 517U * 4U, std::byte{0x5a});
    std::vector<std::string_view> bip39_words(2048U, "unused");
    bip39_words[0] = "abandon";
    bip39_words[3] = "about";
    const std::array<std::string_view, 12> test_mnemonic{
        "abandon", "abandon", "abandon", "abandon", "abandon", "abandon",
        "abandon", "abandon", "abandon", "abandon", "abandon", "about"};

    benchmark("SHA-256", 10000U, [&] { (void)crypto_research::hashing::sha256(bytes); });
    benchmark("RIPEMD-160", 10000U, [&] { (void)crypto_research::hashing::ripemd160(bytes); });
    benchmark("Base58Check", 5000U, [&] { (void)crypto_research::encoding::base58check_encode(public_key); });
    benchmark("secp256k1 public-key generation", 1000U, [&] {
        (void)crypto_research::bitcoin::PrivateKey::from_secret_scalar(test_scalar).public_key();
    });
    benchmark("P2PKH from public key", 5000U, [&] { (void)crypto_research::bitcoin::p2pkh_address(public_key); });
    benchmark("BIP39 checksum validation", 100U, [&] {
        (void)crypto_research::bitcoin::bip39::checksum_is_valid(test_mnemonic, bip39_words);
    });
    benchmark("image low-bit extraction", 10U, [&] {
        (void)crypto_research::steganography::extract_low_bits_cpu(image, 4U);
    });
    (void)p2pkh;
    return 0;
}
