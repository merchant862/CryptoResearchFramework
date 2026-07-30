#include "core/hashing/sha256.hpp"

#include <openssl/evp.h>

#include <stdexcept>

namespace crypto_research::hashing {
namespace {

Sha256Digest digest(std::span<const std::byte> data) {
    Sha256Digest output{};
    unsigned int output_size = 0;
    if (EVP_Digest(data.data(), data.size(), reinterpret_cast<unsigned char*>(output.data()), &output_size, EVP_sha256(), nullptr) != 1 ||
        output_size != output.size()) {
        throw std::runtime_error("OpenSSL could not calculate SHA-256");
    }
    return output;
}

} // namespace

Sha256Digest sha256(std::span<const std::byte> data) { return digest(data); }

Sha256Digest double_sha256(std::span<const std::byte> data) {
    const auto first = digest(data);
    return digest(first);
}

} // namespace crypto_research::hashing
