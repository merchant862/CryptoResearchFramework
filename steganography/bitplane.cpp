#include "steganography/bitplane.hpp"

#include <array>
#include <stdexcept>

namespace crypto_research::steganography {

std::array<std::size_t, 16> low_nibble_histogram(std::span<const std::byte> bytes) {
    std::array<std::size_t, 16> histogram{};
    for (const auto value : bytes) ++histogram[std::to_integer<unsigned char>(value) & 0x0FU];
    return histogram;
}

std::vector<std::byte> extract_low_bits_cpu(std::span<const std::byte> bytes, unsigned int bit_count) {
    if (bit_count == 0 || bit_count > 8) throw std::invalid_argument("bit count must be in the range 1..8");
    const auto mask = static_cast<unsigned char>((1U << bit_count) - 1U);
    std::vector<std::byte> output;
    output.reserve(bytes.size());
    for (const auto value : bytes) output.push_back(std::byte{static_cast<unsigned char>(std::to_integer<unsigned char>(value) & mask)});
    return output;
}

std::vector<std::byte> extract_lsb_bytes(std::span<const std::byte> pixels,
                                         int width,
                                         int height,
                                         int channels,
                                         ChannelOrder channels_to_read,
                                         unsigned int bit_depth,
                                         ScanOrder scan_order) {
    if (width <= 0 || height <= 0 || channels < 3) throw std::invalid_argument("LSB extraction requires an RGB image");
    if (bit_depth == 0 || bit_depth > 4) throw std::invalid_argument("LSB bit depth must be in the range 1..4");
    const auto expected_bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * static_cast<std::size_t>(channels);
    if (pixels.size() != expected_bytes) throw std::invalid_argument("pixel buffer does not match image dimensions");

    std::array<int, 3> selected{};
    std::size_t selected_count = 0;
    // OpenCV stores colour images in BGR(A) byte order.
    switch (channels_to_read) {
    case ChannelOrder::red: selected[selected_count++] = 2; break;
    case ChannelOrder::green: selected[selected_count++] = 1; break;
    case ChannelOrder::blue: selected[selected_count++] = 0; break;
    case ChannelOrder::rgb: selected = {2, 1, 0}; selected_count = 3; break;
    case ChannelOrder::bgr: selected = {0, 1, 2}; selected_count = 3; break;
    }

    std::vector<std::byte> output;
    unsigned char current = 0;
    unsigned int bit_count = 0;
    auto append_bit = [&](unsigned char bit) {
        current = static_cast<unsigned char>((current << 1U) | bit);
        ++bit_count;
        if (bit_count == 8U) {
            output.push_back(std::byte{current});
            current = 0;
            bit_count = 0;
        }
    };
    auto append_pixel = [&](int x, int y) {
        const auto pixel = (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * static_cast<std::size_t>(channels);
        for (std::size_t selection = 0; selection < selected_count; ++selection) {
            const auto value = std::to_integer<unsigned char>(pixels[pixel + static_cast<std::size_t>(selected[selection])]);
            for (unsigned int bit = bit_depth; bit > 0U; --bit) append_bit((value >> (bit - 1U)) & 1U);
        }
    };
    if (scan_order == ScanOrder::xy) {
        for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) append_pixel(x, y);
    } else {
        for (int x = 0; x < width; ++x) for (int y = 0; y < height; ++y) append_pixel(x, y);
    }
    return output;
}

std::vector<AsciiRun> printable_ascii_runs(std::span<const std::byte> bytes, std::size_t minimum_length) {
    if (minimum_length == 0U) throw std::invalid_argument("minimum ASCII run length must be positive");
    std::vector<AsciiRun> runs;
    std::size_t start = 0;
    std::string current;
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        const auto value = std::to_integer<unsigned char>(bytes[index]);
        if (value >= 32U && value <= 126U) {
            if (current.empty()) start = index;
            current.push_back(static_cast<char>(value));
        } else {
            if (current.size() >= minimum_length) runs.push_back({start, std::move(current)});
            current.clear();
        }
    }
    if (current.size() >= minimum_length) runs.push_back({start, std::move(current)});
    return runs;
}

} // namespace crypto_research::steganography
