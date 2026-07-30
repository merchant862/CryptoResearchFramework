#include "core/hashing/ripemd160.hpp"

#include <openssl/evp.h>

#include <stdexcept>

namespace crypto_research::hashing {

Ripemd160Digest ripemd160(std::span<const std::byte> data) {
    Ripemd160Digest output{};
    unsigned int output_size = 0;
    if (EVP_Digest(data.data(), data.size(), reinterpret_cast<unsigned char*>(output.data()), &output_size, EVP_ripemd160(), nullptr) != 1 ||
        output_size != output.size()) {
        throw std::runtime_error("OpenSSL could not calculate RIPEMD-160");
    }
    return output;
}

} // namespace crypto_research::hashing
