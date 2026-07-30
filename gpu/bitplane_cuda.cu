#include "gpu/bitplane.hpp"

#include <cuda_runtime.h>

#include <stdexcept>
#include <string>

namespace crypto_research::gpu {
namespace {

__global__ void extract_low_bits_kernel(const unsigned char* input, unsigned char* output, std::size_t size, unsigned char mask) {
    const auto index = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (index < size) output[index] = input[index] & mask;
}

void check(cudaError_t status, const char* operation) {
    if (status != cudaSuccess) throw std::runtime_error(std::string{operation} + ": " + cudaGetErrorString(status));
}

} // namespace

bool cuda_available() noexcept {
    int device_count = 0;
    return cudaGetDeviceCount(&device_count) == cudaSuccess && device_count > 0;
}

std::vector<std::byte> extract_low_bits(std::span<const std::byte> input, unsigned int bit_count) {
    return extract_low_bits_profiled(input, bit_count).bytes;
}

ExtractionResult extract_low_bits_profiled(std::span<const std::byte> input, unsigned int bit_count) {
    if (bit_count == 0 || bit_count > 8) throw std::invalid_argument("bit count must be in the range 1..8");
    if (input.empty()) return {};

    unsigned char* device_input = nullptr;
    unsigned char* device_output = nullptr;
    cudaEvent_t kernel_start = nullptr;
    cudaEvent_t kernel_end = nullptr;
    try {
        check(cudaMalloc(&device_input, input.size()), "allocating GPU input buffer");
        check(cudaMalloc(&device_output, input.size()), "allocating GPU output buffer");
        check(cudaMemcpy(device_input, input.data(), input.size(), cudaMemcpyHostToDevice), "uploading image bytes");
        check(cudaEventCreate(&kernel_start), "creating CUDA start event");
        check(cudaEventCreate(&kernel_end), "creating CUDA end event");
        constexpr unsigned int threads = 256;
        const auto blocks = static_cast<unsigned int>((input.size() + threads - 1U) / threads);
        const auto mask = static_cast<unsigned char>((1U << bit_count) - 1U);
        check(cudaEventRecord(kernel_start), "recording CUDA start event");
        extract_low_bits_kernel<<<blocks, threads>>>(device_input, device_output, input.size(), mask);
        check(cudaGetLastError(), "launching low-bit extraction kernel");
        check(cudaEventRecord(kernel_end), "recording CUDA end event");
        check(cudaEventSynchronize(kernel_end), "synchronizing CUDA kernel");
        float kernel_milliseconds = 0.0F;
        check(cudaEventElapsedTime(&kernel_milliseconds, kernel_start, kernel_end), "measuring CUDA kernel time");
        std::vector<std::byte> output(input.size());
        check(cudaMemcpy(output.data(), device_output, output.size(), cudaMemcpyDeviceToHost), "downloading extracted bits");
        cudaFree(device_output);
        cudaFree(device_input);
        cudaEventDestroy(kernel_end);
        cudaEventDestroy(kernel_start);
        return {.bytes = std::move(output), .kernel_milliseconds = kernel_milliseconds};
    } catch (...) {
        cudaEventDestroy(kernel_end);
        cudaEventDestroy(kernel_start);
        cudaFree(device_output);
        cudaFree(device_input);
        throw;
    }
}

} // namespace crypto_research::gpu
