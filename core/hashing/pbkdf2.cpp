#include "core/hashing/pbkdf2.hpp"

#include <openssl/evp.h>

#include <limits>
#include <stdexcept>

namespace crypto_research::hashing {

Pbkdf2Sha512Output pbkdf2_hmac_sha512(std::span<const std::byte> password,
                                      std::span<const std::byte> salt,
                                      unsigned int iterations) {
    if (iterations == 0) throw std::invalid_argument("PBKDF2 requires at least one iteration");
    if (password.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        salt.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("PBKDF2 input is too large");
    }
    Pbkdf2Sha512Output output{};
    if (PKCS5_PBKDF2_HMAC(reinterpret_cast<const char*>(password.data()), static_cast<int>(password.size()),
                          reinterpret_cast<const unsigned char*>(salt.data()), static_cast<int>(salt.size()),
                          static_cast<int>(iterations), EVP_sha512(), static_cast<int>(output.size()),
                          reinterpret_cast<unsigned char*>(output.data())) != 1) {
        throw std::runtime_error("OpenSSL could not calculate PBKDF2-HMAC-SHA512");
    }
    return output;
}

} // namespace crypto_research::hashing
