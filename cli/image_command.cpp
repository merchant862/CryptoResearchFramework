#include "cli/image_command.hpp"

#include "gpu/bitplane.hpp"
#include "image_analysis/png_analysis.hpp"

#include <spdlog/spdlog.h>

#include <exception>
#include <string_view>

namespace crypto_research::cli {

int image_command(int argc, char* argv[]) {
    if (argc < 4 || argc > 5) {
        spdlog::error("Usage: crypto_research analyze-image <input.png> <output-directory> [--gpu]");
        return 2;
    }
    const bool use_gpu = argc == 5 && std::string_view{argv[4]} == "--gpu";
    if (argc == 5 && !use_gpu) {
        spdlog::error("Unknown option: {}", argv[4]);
        return 2;
    }
    if (use_gpu && !gpu::cuda_available()) {
        spdlog::error("No CUDA-capable GPU is available in this build/runtime.");
        return 1;
    }
    try {
        const auto evidence = image_analysis::analyse_png(argv[2], use_gpu);
        image_analysis::write_evidence_report(evidence, argv[3]);
        spdlog::info("Analysed {}x{} image ({} channels, {} bytes) in {:.3f} ms; report written to {}",
                     evidence.width, evidence.height, evidence.channels, evidence.decoded_byte_count,
                     evidence.elapsed_milliseconds, argv[3]);
        if (use_gpu) spdlog::info("CUDA kernel time: {:.3f} ms", evidence.gpu_kernel_milliseconds);
        return 0;
    } catch (const std::exception& error) {
        spdlog::error("{}", error.what());
        return 1;
    }
}

} // namespace crypto_research::cli
