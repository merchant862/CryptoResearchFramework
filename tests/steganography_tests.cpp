#include "steganography/bitplane.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> bgr_pixels_with_red_lsb(std::string_view text) {
    std::vector<std::byte> pixels;
    for (const auto character : text) {
        for (int bit = 7; bit >= 0; --bit) {
            pixels.push_back(std::byte{0});
            pixels.push_back(std::byte{0});
            pixels.push_back(std::byte{static_cast<unsigned char>((static_cast<unsigned char>(character) >> bit) & 1U)});
        }
    }
    return pixels;
}

TEST(Steganography, ExtractsRedChannelLsbTextInXyOrder) {
    const auto pixels = bgr_pixels_with_red_lsb("TEST");
    const auto bytes = crypto_research::steganography::extract_lsb_bytes(
        pixels, 32, 1, 3, crypto_research::steganography::ChannelOrder::red, 1,
        crypto_research::steganography::ScanOrder::xy);
    ASSERT_EQ(bytes.size(), 4U);
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[0]), static_cast<unsigned char>('T'));
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[3]), static_cast<unsigned char>('T'));
    const auto runs = crypto_research::steganography::printable_ascii_runs(bytes);
    ASSERT_EQ(runs.size(), 1U);
    EXPECT_EQ(runs[0].text, "TEST");
}

TEST(Steganography, RejectsUnsupportedBitDepth) {
    const std::array<std::byte, 3> pixel{};
    EXPECT_THROW((void)crypto_research::steganography::extract_lsb_bytes(
                     pixel, 1, 1, 3, crypto_research::steganography::ChannelOrder::red, 5,
                     crypto_research::steganography::ScanOrder::xy),
                 std::invalid_argument);
}

} // namespace
