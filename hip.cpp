#include "ppm.hpp"

#include <chrono>
#include <cstdlib>
#include <hip/hip_runtime.h>
#include <iostream>
#include <vector>

constexpr int REQUIRED_ARGC = 3;
constexpr int THREADS_PER_BLOCK = 256;

__global__ void rgbToGrayKernel(
    const unsigned char* rgb,
    unsigned char* gray,
    int pixelCount
) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < pixelCount) {
        int idx = i * RGB_CHANNELS;
        gray[i] = static_cast<unsigned char>(
            LUMA_RED * rgb[idx] + LUMA_GREEN * rgb[idx + 1] + LUMA_BLUE * rgb[idx + 2]
        );
    }
}

#define HIP_CHECK(call) do { \
    hipError_t error = call; \
    if (error != hipSuccess) { \
        std::cerr << hipGetErrorString(error) << '\n'; \
        return EXIT_FAILURE; \
    } \
} while (0)

int main(int argc, char** argv) {
    if (argc < REQUIRED_ARGC) {
        std::cerr << "Usage: " << argv[0] << " input.ppm output.pgm\n";
        return EXIT_FAILURE;
    }

    Image image;
    if (!readPPM(argv[1], image)) {
        return EXIT_FAILURE;
    }

    int pixelCount = image.width * image.height;
    size_t rgbBytes = static_cast<size_t>(pixelCount) * RGB_CHANNELS;
    size_t grayBytes = static_cast<size_t>(pixelCount);

    unsigned char* deviceRGB = nullptr;
    unsigned char* deviceGray = nullptr;
    HIP_CHECK(hipMalloc(&deviceRGB, rgbBytes));
    HIP_CHECK(hipMalloc(&deviceGray, grayBytes));

    std::vector<unsigned char> gray(pixelCount);
    auto start = std::chrono::high_resolution_clock::now();

    HIP_CHECK(hipMemcpy(deviceRGB, image.rgb.data(), rgbBytes, hipMemcpyHostToDevice));

    int gridSize = (pixelCount + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    rgbToGrayKernel<<<gridSize, THREADS_PER_BLOCK>>>(deviceRGB, deviceGray, pixelCount);

    HIP_CHECK(hipGetLastError());
    HIP_CHECK(hipDeviceSynchronize());
    HIP_CHECK(hipMemcpy(gray.data(), deviceGray, grayBytes, hipMemcpyDeviceToHost));

    auto end = std::chrono::high_resolution_clock::now();

    HIP_CHECK(hipFree(deviceRGB));
    HIP_CHECK(hipFree(deviceGray));

    if (!writePGM(argv[2], image.width, image.height, gray)) {
        return EXIT_FAILURE;
    }

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "HIP time: " << elapsed << " seconds\n";
    return EXIT_SUCCESS;
}
