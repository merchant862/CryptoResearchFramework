#include "bitcoin/bip32/xpub.hpp"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {

TEST(Bip32, DerivesPublishedNonHardenedXpubChild) {
    const auto parent = crypto_research::bitcoin::bip32::ExtendedPublicKey::parse(
        "xpub6D4BDPcP2GT577Vvch3R8wDkScZWzQzMMUm3PWbmWvVJrZwQY4VUNgqFJPMM3No2dFDFGTsxxpG5uJh7n7epu4trkrX7x7DogT5Uv6fcLW5");
    EXPECT_EQ(parent.derive_normal(2U).to_xpub(),
              "xpub6FHa3pjLCk84BayeJxFW2SP4XRrFd1JYnxeLeU8EqN3vDfZmbqBqaGJAyiLjTAwm6ZLRQUMv1ZACTj37sR62cfN7fe5JnJ7dh8zL4fiyLHV");
}

TEST(Bip32, RejectsHardenedDerivationFromXpub) {
    const auto xpub = crypto_research::bitcoin::bip32::ExtendedPublicKey::parse(
        "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESFjqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8");
    EXPECT_THROW((void)xpub.derive_normal(0x80000000U), std::invalid_argument);
}

} // namespace
