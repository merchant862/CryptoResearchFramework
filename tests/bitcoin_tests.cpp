#include "bitcoin/p2pkh.hpp"
#include "bitcoin/private_key.hpp"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>

namespace {

TEST(Bitcoin, DerivesKnownCompressedP2pkhAddress) {
    const auto key = crypto_research::bitcoin::PrivateKey::from_hex(
        "0000000000000000000000000000000000000000000000000000000000000001");
    EXPECT_EQ(crypto_research::bitcoin::p2pkh_address(key.public_key()), "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH");
}

TEST(Bitcoin, DerivesKnownUncompressedP2pkhAddress) {
    const auto key = crypto_research::bitcoin::PrivateKey::from_hex(
        "0000000000000000000000000000000000000000000000000000000000000001");
    EXPECT_EQ(crypto_research::bitcoin::p2pkh_address(key.public_key(false)), "1EHNa6Q4Jz2uvNExL497mE43ikXhwF6kZm");
}

TEST(Bitcoin, RejectsOutOfRangePrivateKey) {
    EXPECT_THROW(crypto_research::bitcoin::PrivateKey::from_hex(
                     "0000000000000000000000000000000000000000000000000000000000000000"),
                 std::invalid_argument);
}

TEST(Bitcoin, DerivesPublicAddressFromInternalScalarWithoutExposingIt) {
    std::array<std::byte, 32> scalar{};
    scalar.back() = std::byte{0x01};
    EXPECT_EQ(crypto_research::bitcoin::p2pkh_address_from_secret_scalar(scalar),
              "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH");
}

} // namespace
