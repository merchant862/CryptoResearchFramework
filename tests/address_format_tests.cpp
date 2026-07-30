#include "bitcoin/address/format.hpp"

#include <gtest/gtest.h>

TEST(AddressFormat, ClassifiesValidPublicFormats) {
    auto p = crypto_research::bitcoin::address::inspect_public_address(" 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH ");
    EXPECT_TRUE(p.valid);
    EXPECT_EQ(p.type, crypto_research::bitcoin::address::PublicAddressType::p2pkh);
    EXPECT_EQ(p.normalized, "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH");

    auto b = crypto_research::bitcoin::address::inspect_public_address("BC1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4");
    EXPECT_TRUE(b.valid);
    EXPECT_EQ(b.type, crypto_research::bitcoin::address::PublicAddressType::bech32);
    EXPECT_EQ(b.normalized, "bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4");

    auto invalid_checksum =
        crypto_research::bitcoin::address::inspect_public_address("bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t5");
    EXPECT_FALSE(invalid_checksum.valid);
    EXPECT_EQ(invalid_checksum.type, crypto_research::bitcoin::address::PublicAddressType::bech32);

    auto mixed_case =
        crypto_research::bitcoin::address::inspect_public_address("bc1QW508D6QEJXTDG4Y5R3ZARVARY0C5XW7KV8F3T4");
    EXPECT_FALSE(mixed_case.valid);
    EXPECT_EQ(mixed_case.type, crypto_research::bitcoin::address::PublicAddressType::bech32);

    EXPECT_FALSE(crypto_research::bitcoin::address::inspect_public_address("not-an-address").valid);
}
