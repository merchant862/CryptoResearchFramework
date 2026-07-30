#include "image_analysis/png_analysis.hpp"

#include "gpu/bitplane.hpp"
#include "steganography/bitplane.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>

namespace crypto_research::image_analysis {
namespace {

constexpr std::array<unsigned char, 8> png_signature{0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};

std::vector<std::byte> read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("could not open image file: " + path.string());
    const std::vector<char> characters{std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
    std::vector<std::byte> bytes;
    bytes.reserve(characters.size());
    for (const char character : characters) bytes.push_back(std::byte{static_cast<unsigned char>(character)});
    return bytes;
}

struct PngChunkMetadata final {
    std::vector<PngChunk> chunks;
    std::vector<std::byte> significant_bits;
};

PngChunkMetadata inspect_png_chunks(const std::vector<std::byte>& file_bytes) {
    if (file_bytes.size() < png_signature.size()) return {};
    for (std::size_t index = 0; index < png_signature.size(); ++index) {
        if (std::to_integer<unsigned char>(file_bytes[index]) != png_signature[index]) return {};
    }
    PngChunkMetadata metadata;
    for (std::size_t offset = 8; offset + 12 <= file_bytes.size();) {
        const auto length = (static_cast<std::size_t>(std::to_integer<unsigned char>(file_bytes[offset])) << 24U) |
                            (static_cast<std::size_t>(std::to_integer<unsigned char>(file_bytes[offset + 1])) << 16U) |
                            (static_cast<std::size_t>(std::to_integer<unsigned char>(file_bytes[offset + 2])) << 8U) |
                            std::to_integer<unsigned char>(file_bytes[offset + 3]);
        if (length > file_bytes.size() - offset - 12) throw std::runtime_error("PNG contains a truncated chunk");
        const std::string type{
            static_cast<char>(std::to_integer<unsigned char>(file_bytes[offset + 4])),
            static_cast<char>(std::to_integer<unsigned char>(file_bytes[offset + 5])),
            static_cast<char>(std::to_integer<unsigned char>(file_bytes[offset + 6])),
            static_cast<char>(std::to_integer<unsigned char>(file_bytes[offset + 7]))};
        metadata.chunks.push_back({type, length, offset});
        const bool is_sbit = std::to_integer<unsigned char>(file_bytes[offset + 4]) == 's' &&
                             std::to_integer<unsigned char>(file_bytes[offset + 5]) == 'B' &&
                             std::to_integer<unsigned char>(file_bytes[offset + 6]) == 'I' &&
                             std::to_integer<unsigned char>(file_bytes[offset + 7]) == 'T';
        if (is_sbit) metadata.significant_bits = {file_bytes.begin() + static_cast<std::ptrdiff_t>(offset + 8), file_bytes.begin() + static_cast<std::ptrdiff_t>(offset + 8 + length)};
        offset += length + 12;
    }
    return metadata;
}

} // namespace

PngEvidence analyse_png(const std::filesystem::path& path, bool use_gpu) {
    const auto start = std::chrono::steady_clock::now();
    const auto file_bytes = read_file(path);
    cv::Mat image = cv::imread(path.string(), cv::IMREAD_UNCHANGED);
    if (image.empty()) throw std::runtime_error("OpenCV could not decode the PNG image");
    if (image.depth() != CV_8U) throw std::runtime_error("only 8-bit PNG samples are currently supported");
    if (!image.isContinuous()) image = image.clone();

    const auto byte_count = image.total() * image.elemSize();
    const auto image_bytes = std::span{reinterpret_cast<const std::byte*>(image.data), byte_count};
    const auto metadata = inspect_png_chunks(file_bytes);
    const auto gpu_result = use_gpu ? gpu::extract_low_bits_profiled(image_bytes, 4) : gpu::ExtractionResult{};
    static constexpr std::array<std::string_view, 4> channel_names{"blue", "green", "red", "alpha"};
    std::vector<std::pair<std::string, std::vector<std::byte>>> bit_planes;
    bit_planes.reserve(static_cast<std::size_t>(image.channels()) * 8U);
    for (int channel = 0; channel < image.channels(); ++channel) {
        const auto& channel_name = channel_names[static_cast<std::size_t>(channel) < channel_names.size()
                                                     ? static_cast<std::size_t>(channel)
                                                     : 0U];
        for (unsigned int bit = 0; bit < 8; ++bit) {
            std::vector<std::byte> plane;
            plane.reserve(image.total());
            for (std::size_t pixel = 0; pixel < image.total(); ++pixel) {
                const auto value = std::to_integer<unsigned char>(image_bytes[pixel * static_cast<std::size_t>(image.channels()) + static_cast<std::size_t>(channel)]);
                plane.push_back(std::byte{static_cast<unsigned char>(((value >> bit) & 1U) == 0U ? 0U : 255U)});
            }
            bit_planes.emplace_back(std::string{channel_name} + "_bit" + std::to_string(bit), std::move(plane));
        }
    }
    PngEvidence evidence{
        .width = image.cols,
        .height = image.rows,
        .channels = image.channels(),
        .png_chunks = metadata.chunks,
        .significant_bits = metadata.significant_bits,
        .low_nibble_histogram = steganography::low_nibble_histogram(image_bytes),
        .low_four_bits = use_gpu ? gpu_result.bytes : steganography::extract_low_bits_cpu(image_bytes, 4),
        .decoded_pixels = {image_bytes.begin(), image_bytes.end()},
        .bit_planes = std::move(bit_planes),
        .decoded_byte_count = byte_count,
        .gpu_kernel_milliseconds = gpu_result.kernel_milliseconds,
        .used_gpu = use_gpu,
    };
    evidence.elapsed_milliseconds = std::chrono::duration<double, std::milli>{std::chrono::steady_clock::now() - start}.count();
    return evidence;
}

void write_evidence_report(const PngEvidence& evidence, const std::filesystem::path& output_directory) {
    std::filesystem::create_directories(output_directory);
    {
        std::ofstream report(output_directory / "report.txt");
        if (!report) throw std::runtime_error("could not create evidence report");
        report << "width: " << evidence.width << '\n';
        report << "height: " << evidence.height << '\n';
        report << "channels: " << evidence.channels << '\n';
        report << "decoded_bytes: " << evidence.decoded_byte_count << '\n';
        report << "elapsed_ms: " << evidence.elapsed_milliseconds << '\n';
        report << "execution: " << (evidence.used_gpu ? "gpu" : "cpu") << '\n';
        if (evidence.used_gpu) report << "gpu_kernel_ms: " << evidence.gpu_kernel_milliseconds << '\n';
        report << "png_chunks:\n";
        for (const auto& chunk : evidence.png_chunks) report << "  " << chunk.type << ':' << chunk.size << " @" << chunk.offset << '\n';
        report << "sBIT:";
        for (const auto value : evidence.significant_bits) report << ' ' << std::to_integer<unsigned char>(value);
        report << "\nlow_nibble_histogram:\n";
        for (std::size_t index = 0; index < evidence.low_nibble_histogram.size(); ++index) {
            report << std::hex << index << std::dec << ": " << evidence.low_nibble_histogram[index] << '\n';
        }
    }
    std::ofstream low_bits(output_directory / "low4.raw", std::ios::binary);
    if (!low_bits) throw std::runtime_error("could not create low-bit evidence file");
    low_bits.write(reinterpret_cast<const char*>(evidence.low_four_bits.data()), static_cast<std::streamsize>(evidence.low_four_bits.size()));

    const auto cv_type = evidence.channels == 4 ? CV_8UC4 : CV_8UC3;
    for (unsigned int bit_count = 1; bit_count <= 4; ++bit_count) {
        auto residual = steganography::extract_low_bits_cpu(evidence.decoded_pixels, bit_count);
        const auto scale = static_cast<unsigned char>(255U / ((1U << bit_count) - 1U));
        for (auto& byte : residual) byte = std::byte{static_cast<unsigned char>(std::to_integer<unsigned char>(byte) * scale)};
        cv::Mat residual_image(evidence.height, evidence.width, cv_type, residual.data());
        if (!cv::imwrite((output_directory / ("low" + std::to_string(bit_count) + "_residual.png")).string(), residual_image)) {
            throw std::runtime_error("could not write low-bit residual image");
        }
    }

    const auto bitplane_directory = output_directory / "bitplanes";
    std::filesystem::create_directories(bitplane_directory);
    std::ofstream manifest(bitplane_directory / "manifest.txt");
    if (!manifest) throw std::runtime_error("could not create bit-plane manifest");
    manifest << "Each PNG is grayscale: white means that source-channel bit is set.\n";
    cv::Mat mosaic(evidence.height * evidence.channels, evidence.width * 8, CV_8UC1, cv::Scalar{0});
    for (std::size_t plane_index = 0; plane_index < evidence.bit_planes.size(); ++plane_index) {
        const auto& [name, plane] = evidence.bit_planes[plane_index];
        cv::Mat bit_plane(evidence.height, evidence.width, CV_8UC1,
                          const_cast<std::byte*>(plane.data()));
        const auto output = bitplane_directory / (name + ".png");
        if (!cv::imwrite(output.string(), bit_plane)) {
            throw std::runtime_error("could not write bit-plane: " + output.string());
        }
        const auto channel = static_cast<int>(plane_index / 8U);
        const auto bit = static_cast<int>(plane_index % 8U);
        bit_plane.copyTo(mosaic(cv::Rect{bit * evidence.width, channel * evidence.height,
                                        evidence.width, evidence.height}));
        manifest << output.filename().string() << '\n';
    }
    if (!cv::imwrite((bitplane_directory / "all_channels_bitplanes.png").string(), mosaic)) {
        throw std::runtime_error("could not write bit-plane contact sheet");
    }
}

} // namespace crypto_research::image_analysis
