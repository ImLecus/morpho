// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/jpeg_to_png.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: morpho_jpeg_to_png_test <input.jpg>\n";
        return 2;
    }

    const std::filesystem::path input = argv[1];
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "morpho-jpeg-to-png-test.png";

    std::error_code error;
    std::filesystem::remove(output, error);

    morpho::image::JpegToPngConverter converter;
    morpho::ConversionRequest request;
    request.inputPath = input;
    request.outputPath = output;

    const morpho::ConversionResult result = converter.convert(request);
    if (!result.success) {
        std::cerr << "Conversion failed: " << result.error << '\n';
        return 1;
    }

    std::ifstream file(output, std::ios::binary);
    if (!file) {
        std::cerr << "Could not open the generated PNG.\n";
        return 1;
    }

    std::array<unsigned char, 8> signature{};
    file.read(reinterpret_cast<char *>(signature.data()),
              static_cast<std::streamsize>(signature.size()));
    constexpr std::array<unsigned char, 8> kPngSignature = {0x89, 'P',  'N',  'G',
                                                            0x0D, 0x0A, 0x1A, 0x0A};

    if (!file || signature != kPngSignature) {
        std::cerr << "The generated file is not a PNG.\n";
        std::filesystem::remove(output, error);
        return 1;
    }

    std::array<unsigned char, 16> header{};
    file.read(reinterpret_cast<char *>(header.data()), static_cast<std::streamsize>(header.size()));
    const auto readUint32 = [](const unsigned char *bytes) {
        return (static_cast<std::uint32_t>(bytes[0]) << 24U) |
               (static_cast<std::uint32_t>(bytes[1]) << 16U) |
               (static_cast<std::uint32_t>(bytes[2]) << 8U) | static_cast<std::uint32_t>(bytes[3]);
    };

    if (!file || header[4] != 'I' || header[5] != 'H' || header[6] != 'D' || header[7] != 'R' ||
        readUint32(header.data() + 8) != 8 || readUint32(header.data() + 12) != 8) {
        std::cerr << "The generated PNG has an unexpected header.\n";
        std::filesystem::remove(output, error);
        return 1;
    }

    const std::uintmax_t size = std::filesystem::file_size(output, error);
    std::filesystem::remove(output, error);
    if (error || size <= signature.size()) {
        std::cerr << "The generated PNG is empty.\n";
        return 1;
    }

    std::cout << "Converted " << input << " to a PNG of " << size << " bytes.\n";
    return 0;
}
