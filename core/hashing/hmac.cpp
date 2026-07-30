#include "core/hashing/hmac.hpp"

#include <openssl/hmac.h>

#include <stdexcept>

namespace crypto_research::hashing {

HmacSha512Digest hmac_sha512(std::span<const std::byte> key, std::span<const std::byte> message) {
    HmacSha512Digest output{};
    unsigned int output_length = 0;
    if (HMAC(EVP_sha512(), key.data(), static_cast<int>(key.size()),
             reinterpret_cast<const unsigned char*>(message.data()), message.size(),
             reinterpret_cast<unsigned char*>(output.data()), &output_length) == nullptr || output_length != output.size()) {
        throw std::runtime_error("OpenSSL could not calculate HMAC-SHA512");
    }
    return output;
}

} // namespace crypto_research::hashing
