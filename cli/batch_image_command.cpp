#include "cli/batch_image_command.hpp"

#include "core/utils/progress.hpp"
#include "core/utils/thread_pool.hpp"
#include "gpu/bitplane.hpp"
#include "image_analysis/png_analysis.hpp"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace crypto_research::cli {
namespace {

struct Options final {
    std::size_t workers = std::thread::hardware_concurrency();
    bool use_gpu = false;
};

Options parse_options(int argc, char* argv[]) {
    Options options;
    for (int index = 4; index < argc; ++index) {
        const std::string_view option{argv[index]};
        if (option == "--gpu") {
            options.use_gpu = true;
        } else if (option == "--workers" && index + 1 < argc) {
            options.workers = static_cast<std::size_t>(std::stoul(argv[++index]));
            if (options.workers == 0) throw std::invalid_argument("worker count must be at least one");
        } else {
            throw std::invalid_argument("unknown batch option: " + std::string{option});
        }
    }
    return options;
}

} // namespace

int batch_image_command(int argc, char* argv[]) {
    if (argc < 4) {
        spdlog::error("Usage: crypto_research batch-analyze <input-directory> <output-directory> [--workers N] [--gpu]");
        return 2;
    }
    try {
        const auto options = parse_options(argc, argv);
        if (options.use_gpu && !gpu::cuda_available()) throw std::runtime_error("no CUDA-capable GPU is available");

        std::vector<std::filesystem::path> images;
        for (const auto& entry : std::filesystem::directory_iterator{argv[2]}) {
            if (entry.is_regular_file() && entry.path().extension() == ".png") images.push_back(entry.path());
        }
        std::sort(images.begin(), images.end());
        if (images.empty()) throw std::runtime_error("input directory contains no PNG files");

        std::filesystem::create_directories(argv[3]);
        utils::ThreadPool pool{options.workers};
        utils::Progress progress{images.size()};
        std::vector<std::future<image_analysis::PngEvidence>> futures;
        futures.reserve(images.size());
        for (const auto& image : images) {
            futures.push_back(pool.submit([&progress, image, use_gpu = options.use_gpu] {
                try {
                    auto evidence = image_analysis::analyse_png(image, use_gpu);
                    progress.complete_one();
                    return evidence;
                } catch (...) {
                    progress.complete_one();
                    throw;
                }
            }));
        }

        while (progress.completed() < progress.total()) {
            spdlog::info("Progress: {}/{} images", progress.completed(), progress.total());
            std::this_thread::sleep_for(std::chrono::milliseconds{250});
        }

        nlohmann::json report = nlohmann::json::array();
        for (std::size_t index = 0; index < futures.size(); ++index) {
            const auto evidence = futures[index].get();
            const auto output = std::filesystem::path{argv[3]} / (std::to_string(index) + "_" + images[index].stem().string());
            image_analysis::write_evidence_report(evidence, output);
            report.push_back({
                {"file", images[index].filename().string()},
                {"decoded_bytes", evidence.decoded_byte_count},
                {"elapsed_ms", evidence.elapsed_milliseconds},
                {"gpu_kernel_ms", evidence.gpu_kernel_milliseconds},
                {"used_gpu", evidence.used_gpu}
            });
        }
        std::ofstream summary{std::filesystem::path{argv[3]} / "batch-report.json"};
        summary << report.dump(2) << '\n';
        spdlog::info("Completed {} images using {} CPU workers{}.", images.size(), pool.worker_count(), options.use_gpu ? " and CUDA" : "");
        return 0;
    } catch (const std::exception& error) {
        spdlog::error("{}", error.what());
        return 1;
    }
}

} // namespace crypto_research::cli
