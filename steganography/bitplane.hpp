#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <span>
#include <vector>

namespace crypto_research::steganography {

[[nodiscard]] std::array<std::size_t, 16> low_nibble_histogram(std::span<const std::byte> bytes);
[[nodiscard]] std::vector<std::byte> extract_low_bits_cpu(std::span<const std::byte> bytes, unsigned int bit_count);

enum class ChannelOrder { red, green, blue, rgb, bgr };
enum class ScanOrder { xy, yx };

struct AsciiRun final {
    std::size_t byte_offset{};
    std::string text;
};

[[nodiscard]] std::vector<std::byte> extract_lsb_bytes(std::span<const std::byte> pixels,
                                                        int width,
                                                        int height,
                                                        int channels,
                                                        ChannelOrder channels_to_read,
                                                        unsigned int bit_depth,
                                                        ScanOrder scan_order);
[[nodiscard]] std::vector<AsciiRun> printable_ascii_runs(std::span<const std::byte> bytes,
                                                          std::size_t minimum_length = 4U);

} // namespace crypto_research::steganography
