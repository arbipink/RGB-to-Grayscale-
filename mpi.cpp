#include "ppm.hpp"

#include <cstdlib>
#include <iostream>
#include <mpi.h>
#include <vector>

constexpr int REQUIRED_ARGC = 3;
constexpr int ROOT_RANK = 0;

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0;
    int processCount = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &processCount);

    if (argc < REQUIRED_ARGC) {
        if (rank == ROOT_RANK) {
            std::cerr << "Usage: " << argv[0] << " input.ppm output.pgm\n";
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    Image image;
    int width = 0;
    int height = 0;
    int inputOk = 1;

    if (rank == ROOT_RANK) {
        if (!readPPM(argv[1], image)) {
            inputOk = 0;
        } else {
            width = image.width;
            height = image.height;
        }
    }

    MPI_Bcast(&inputOk, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
    if (!inputOk) {
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    MPI_Bcast(&width, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);

    int totalPixels = width * height;
    int basePixels = totalPixels / processCount;
    int remainder = totalPixels % processCount;
    int localPixels = basePixels + (rank < remainder ? 1 : 0);

    std::vector<int> sendCounts(processCount);
    std::vector<int> sendDispls(processCount);
    std::vector<int> recvCounts(processCount);
    std::vector<int> recvDispls(processCount);

    int offset = 0;
    for (int i = 0; i < processCount; ++i) {
        int pixels = basePixels + (i < remainder ? 1 : 0);
        sendCounts[i] = pixels * RGB_CHANNELS;
        sendDispls[i] = offset * RGB_CHANNELS;
        recvCounts[i] = pixels;
        recvDispls[i] = offset;
        offset += pixels;
    }

    std::vector<unsigned char> localRGB(localPixels * RGB_CHANNELS);
    std::vector<unsigned char> localGray(localPixels);
    std::vector<unsigned char> gray;
    if (rank == ROOT_RANK) {
        gray.resize(totalPixels);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    double start = MPI_Wtime();

    MPI_Scatterv(
        rank == ROOT_RANK ? image.rgb.data() : nullptr,
        sendCounts.data(), sendDispls.data(), MPI_UNSIGNED_CHAR,
        localRGB.data(), localPixels * RGB_CHANNELS, MPI_UNSIGNED_CHAR,
        ROOT_RANK, MPI_COMM_WORLD
    );

    for (int i = 0; i < localPixels; ++i) {
        int idx = i * RGB_CHANNELS;
        localGray[i] = rgbToGrayPixel(localRGB[idx], localRGB[idx + 1], localRGB[idx + 2]);
    }

    MPI_Gatherv(
        localGray.data(), localPixels, MPI_UNSIGNED_CHAR,
        rank == ROOT_RANK ? gray.data() : nullptr,
        recvCounts.data(), recvDispls.data(), MPI_UNSIGNED_CHAR,
        ROOT_RANK, MPI_COMM_WORLD
    );

    double localTime = MPI_Wtime() - start;
    double elapsed = 0.0;
    MPI_Reduce(&localTime, &elapsed, 1, MPI_DOUBLE, MPI_MAX, ROOT_RANK, MPI_COMM_WORLD);

    int outputOk = 1;
    if (rank == ROOT_RANK) {
        if (!writePGM(argv[2], width, height, gray)) {
            outputOk = 0;
        } else {
            std::cout << "MPI time: " << elapsed << " seconds\n";
        }
    }

    MPI_Bcast(&outputOk, 1, MPI_INT, ROOT_RANK, MPI_COMM_WORLD);
    MPI_Finalize();
    return outputOk ? EXIT_SUCCESS : EXIT_FAILURE;
}
