#include "core/hashing/ripemd160.hpp"
#include "core/hashing/sha256.hpp"
#include "core/utils/hex.hpp"

#include <gtest/gtest.h>

#include <span>
#include <string_view>

namespace {

TEST(Hashing, Sha256MatchesKnownVector) {
    constexpr std::string_view input = "abc";
    const auto bytes = std::as_bytes(std::span{input.data(), input.size()});
    EXPECT_EQ(crypto_research::utils::to_hex(crypto_research::hashing::sha256(bytes)),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(Hashing, Ripemd160MatchesKnownVector) {
    constexpr std::string_view input = "abc";
    const auto bytes = std::as_bytes(std::span{input.data(), input.size()});
    EXPECT_EQ(crypto_research::utils::to_hex(crypto_research::hashing::ripemd160(bytes)),
              "8eb208f7e05d987a9b044a8e98c6b087f15a0bfc");
}

} // namespace
