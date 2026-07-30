#pragma once

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace crypto_research::image_analysis {

struct PngChunk final { std::string type; std::size_t size{}; std::size_t offset{}; };

struct PngEvidence final {
    int width{};
    int height{};
    int channels{};
    std::vector<PngChunk> png_chunks;
    std::vector<std::byte> significant_bits;
    std::array<std::size_t, 16> low_nibble_histogram{};
    std::vector<std::byte> low_four_bits;
    std::vector<std::byte> decoded_pixels;
    // One 8-bit grayscale image per source channel and bit position.  Pixels are
    // either 0 or 255 so each plane can be inspected without a special viewer.
    std::vector<std::pair<std::string, std::vector<std::byte>>> bit_planes;
    std::size_t decoded_byte_count{};
    double elapsed_milliseconds = 0.0;
    double gpu_kernel_milliseconds = 0.0;
    bool used_gpu = false;
};

[[nodiscard]] PngEvidence analyse_png(const std::filesystem::path& path, bool use_gpu = false);
void write_evidence_report(const PngEvidence& evidence, const std::filesystem::path& output_directory);

} // namespace crypto_research::image_analysis
