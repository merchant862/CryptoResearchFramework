#include "core/encoding/base58.hpp"
#include "core/utils/hex.hpp"

#include <gtest/gtest.h>

namespace {

TEST(Base58, PreservesLeadingZeroes) {
    const auto bytes = crypto_research::utils::from_hex("00000001");
    EXPECT_EQ(crypto_research::encoding::base58_encode(bytes), "1112");
}

TEST(Base58, EncodesBase58CheckVector) {
    const auto payload = crypto_research::utils::from_hex("00010966776006953d5567439e5e39f86a0d273bee");
    EXPECT_EQ(crypto_research::encoding::base58check_encode(payload), "16UwLL9Risc3QfPqBUvKofHmBQ7wMtjvM");
}

} // namespace
