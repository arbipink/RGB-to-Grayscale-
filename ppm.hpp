#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

constexpr int RGB_CHANNELS = 3;
constexpr int MAX_PIXEL_VALUE = 255;

// Koefisien luminansi standar ITU-R BT.601 untuk konversi RGB ke Grayscale
constexpr double LUMA_RED = 0.299;
constexpr double LUMA_GREEN = 0.587;
constexpr double LUMA_BLUE = 0.114;

constexpr const char* PPM_HEADER = "P6";
constexpr const char* PGM_HEADER = "P5";
constexpr char PPM_COMMENT_CHAR = '#';

struct Image {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> rgb;
};

inline unsigned char rgbToGrayPixel(unsigned char r, unsigned char g, unsigned char b) {
    return static_cast<unsigned char>(LUMA_RED * r + LUMA_GREEN * g + LUMA_BLUE * b);
}

inline bool nextToken(std::ifstream& file, std::string& token) {
    while (file >> token) {
        if (!token.empty() && token[0] == PPM_COMMENT_CHAR) {
            std::string comment;
            std::getline(file, comment);
            continue;
        }
        return true;
    }
    return false;
}

inline bool readPPM(const char* filename, Image& image) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Gagal membuka file input: " << filename << '\n';
        return false;
    }

    std::string header;
    std::string widthToken;
    std::string heightToken;
    std::string maxValToken;

    if (!nextToken(file, header) || header != PPM_HEADER ||
        !nextToken(file, widthToken) || !nextToken(file, heightToken) ||
        !nextToken(file, maxValToken)) {
        std::cerr << "Format PPM tidak valid (harus P6)\n";
        return false;
    }

    image.width = std::stoi(widthToken);
    image.height = std::stoi(heightToken);
    if (std::stoi(maxValToken) != MAX_PIXEL_VALUE) {
        std::cerr << "PPM harus menggunakan max value " << MAX_PIXEL_VALUE << '\n';
        return false;
    }

    file.get(); // Lewati satu byte whitespace setelah header
    size_t totalBytes = static_cast<size_t>(image.width) * image.height * RGB_CHANNELS;
    image.rgb.resize(totalBytes);
    file.read(reinterpret_cast<char*>(image.rgb.data()), totalBytes);

    if (!file) {
        std::cerr << "Data gambar PPM tidak lengkap\n";
        return false;
    }

    return true;
}

inline bool writePGM(
    const char* filename,
    int width,
    int height,
    const std::vector<unsigned char>& gray
) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Gagal membuka file output: " << filename << '\n';
        return false;
    }

    file << PGM_HEADER << '\n' << width << ' ' << height << '\n' << MAX_PIXEL_VALUE << '\n';
    file.write(reinterpret_cast<const char*>(gray.data()), gray.size());
    return static_cast<bool>(file);
}
