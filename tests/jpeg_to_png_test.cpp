// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/jpeg_to_png.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: morpho_jpeg_to_png_test <input.jpg>\n";
        return 2;
    }

    const std::filesystem::path input = argv[1];

    morpho::image::JpegToPngConverter converter;
    morpho::ConversionRequest request;
    request.inputPath = input;

    const morpho::ConversionResult result = converter.convert(request);
    if (!result.success) {
        std::cerr << "Conversion failed: " << result.error << '\n';
        return 1;
    }

    if (result.outputFileName != "tiny.png") {
        std::cerr << "Unexpected output file name: " << result.outputFileName << '\n';
        return 1;
    }

    constexpr std::array<std::uint8_t, 8> kPngSignature = {0x89, 'P',  'N',  'G',
                                                           0x0D, 0x0A, 0x1A, 0x0A};
    if (result.data.size() < 24 ||
        !std::equal(kPngSignature.begin(), kPngSignature.end(), result.data.begin())) {
        std::cerr << "The generated data is not a PNG.\n";
        return 1;
    }

    const auto readUint32 = [&result](std::size_t offset) {
        return (static_cast<std::uint32_t>(result.data[offset]) << 24U) |
               (static_cast<std::uint32_t>(result.data[offset + 1]) << 16U) |
               (static_cast<std::uint32_t>(result.data[offset + 2]) << 8U) |
               static_cast<std::uint32_t>(result.data[offset + 3]);
    };

    if (result.data[12] != 'I' || result.data[13] != 'H' || result.data[14] != 'D' ||
        result.data[15] != 'R' || readUint32(16) != 8 || readUint32(20) != 8) {
        std::cerr << "The generated PNG has an unexpected header.\n";
        return 1;
    }

    std::cout << "Converted " << input << " to " << result.outputFileName << " ("
              << result.data.size() << " bytes) in memory.\n";
    return 0;
}
