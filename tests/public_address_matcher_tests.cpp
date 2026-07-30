#include "bitcoin/address/public_address_matcher.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace {

TEST(PublicAddressMatcher, ComparesOnlyPublicAddressLines) {
    const auto path = std::filesystem::temp_directory_path() / "crypto-research-public-addresses.txt";
    { std::ofstream output(path, std::ios::trunc); output << "1BoatSLRHtKNngkdXEeobR76b53LETtpyT\n\n1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH\n"; }
    const auto results = crypto_research::bitcoin::address::compare_public_address_file(
        path, "1BgGZ9tcN4rm9KBzDn7KprQz87SZ26SAMH");
    ASSERT_EQ(results.size(), 2U);
    EXPECT_EQ(results[0].line_number, 1U);
    EXPECT_FALSE(results[0].matches);
    EXPECT_EQ(results[1].line_number, 3U);
    EXPECT_TRUE(results[1].matches);
    std::filesystem::remove(path);
}

} // namespace
