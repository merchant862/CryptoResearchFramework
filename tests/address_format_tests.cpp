#include "bitcoin/address/format.hpp"
#include <gtest/gtest.h>
TEST(AddressFormat, ClassifiesValidPublicFormats) {
 auto p=crypto_research::bitcoin::address::inspect_public_address(" 1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH "); EXPECT_TRUE(p.valid); EXPECT_EQ(p.type,crypto_research::bitcoin::address::PublicAddressType::p2pkh); EXPECT_EQ(p.normalized,"1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH");
 auto b=crypto_research::bitcoin::address::inspect_public_address("bc1qw508d6qejxtdg4y5r3zarvary0c5xw7kygt080"); EXPECT_TRUE(b.valid); EXPECT_EQ(b.type,crypto_research::bitcoin::address::PublicAddressType::bech32);
 EXPECT_FALSE(crypto_research::bitcoin::address::inspect_public_address("not-an-address").valid);
}
