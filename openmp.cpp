#include "ppm.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <vector>

constexpr int REQUIRED_ARGC = 3;

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
    std::vector<unsigned char> gray(pixelCount);

    auto start = std::chrono::high_resolution_clock::now();
    #pragma omp parallel for
    for (int i = 0; i < pixelCount; ++i) {
        int idx = i * RGB_CHANNELS;
        gray[i] = rgbToGrayPixel(image.rgb[idx], image.rgb[idx + 1], image.rgb[idx + 2]);
    }
    auto end = std::chrono::high_resolution_clock::now();

    if (!writePGM(argv[2], image.width, image.height, gray)) {
        return EXIT_FAILURE;
    }

    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "OpenMP time: " << elapsed << " seconds\n";
    return EXIT_SUCCESS;
}
