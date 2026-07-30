#include "image_analysis/png_analysis.hpp"
#include "steganography/bitplane.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

int main(int argc, char* argv[]) {
    if (argc != 5 || std::string_view{argv[1]} != "--image" || std::string_view{argv[3]} != "--out") {
        std::cerr << "Usage: png_report --image <input.png> --out <output-folder>\n";
        return 2;
    }
    try {
        const std::filesystem::path image = argv[2];
        const std::filesystem::path output = argv[4];
        const auto evidence = crypto_research::image_analysis::analyse_png(image);
        crypto_research::image_analysis::write_evidence_report(evidence, output);
        nlohmann::json report{{"image_path", image.string()}, {"width", evidence.width}, {"height", evidence.height},
                              {"channels", evidence.channels}, {"has_sbit", !evidence.significant_bits.empty()},
                              {"artifacts", nlohmann::json::array({"report.txt", "low4.raw", "low1_residual.png", "low2_residual.png", "low3_residual.png", "low4_residual.png", "bitplanes/all_channels_bitplanes.png"})}};
        report["chunks"] = nlohmann::json::array();
        for (const auto& chunk : evidence.png_chunks) report["chunks"].push_back({{"type", chunk.type}, {"size", chunk.size}, {"offset", chunk.offset}});
        nlohmann::json ascii = nlohmann::json::array();
        std::ofstream text(output / "extracted_text.txt");
        for (const auto channel : {crypto_research::steganography::ChannelOrder::rgb, crypto_research::steganography::ChannelOrder::bgr}) {
            for (unsigned int depth = 1; depth <= 4; ++depth) for (const auto scan : {crypto_research::steganography::ScanOrder::xy, crypto_research::steganography::ScanOrder::yx}) {
                const auto bytes = crypto_research::steganography::extract_lsb_bytes(evidence.decoded_pixels, evidence.width, evidence.height, evidence.channels, channel, depth, scan);
                for (const auto& run : crypto_research::steganography::printable_ascii_runs(bytes)) {
                    const auto order = channel == crypto_research::steganography::ChannelOrder::rgb ? "RGB" : "BGR";
                    const auto traversal = scan == crypto_research::steganography::ScanOrder::xy ? "XY" : "YX";
                    text << order << " depth=" << depth << " scan=" << traversal << " offset=" << run.byte_offset << " text=" << run.text << '\n';
                    ascii.push_back({{"channel_order", order}, {"bit_depth", depth}, {"scan_order", traversal}, {"offset", run.byte_offset}, {"text", run.text}});
                }
            }
        }
        report["ascii_runs"] = std::move(ascii);
        std::ofstream json_output(output / "report.json");
        json_output << report.dump(2) << '\n';
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
