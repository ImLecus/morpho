// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/image_converter.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {

bool hasPngSignature(const std::vector<std::uint8_t> &data) {
    constexpr std::array<std::uint8_t, 8> kPng = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    return data.size() >= kPng.size() && std::equal(kPng.begin(), kPng.end(), data.begin());
}

bool writeFile(const std::filesystem::path &path, const std::vector<std::uint8_t> &data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char *>(data.data()),
               static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(file);
}

} // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: morpho_image_conversion_test <input.jpg>\n";
        return 2;
    }

    const std::filesystem::path fixture = argv[1];
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "morpho-image-conversion-test";

    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr << "Could not create the test directory.\n";
        return 1;
    }

    morpho::image::ImageConverter converter;
    const std::array<std::string, 7> formats = {"png",  "jpeg", "webp", "avif",
                                                "tiff", "bmp",  "gif"};

    bool ok = true;
    for (const std::string &format : formats) {
        morpho::ConversionRequest encodeRequest;
        encodeRequest.inputPath = fixture;
        encodeRequest.outputFormat = format;

        const morpho::ConversionResult encoded = converter.convert(encodeRequest);
        if (!encoded.success || encoded.data.empty() || encoded.inputBytes == 0 ||
            encoded.inputFormat != "jpeg" || encoded.outputFormat != format) {
            std::cerr << "Could not encode " << format << ": " << encoded.error << '\n';
            ok = false;
            continue;
        }

        const std::filesystem::path encodedPath = directory / encoded.outputFileName;
        if (!writeFile(encodedPath, encoded.data)) {
            std::cerr << "Could not write the " << format << " test file.\n";
            ok = false;
            continue;
        }

        morpho::ConversionRequest decodeRequest;
        decodeRequest.inputPath = encodedPath;
        decodeRequest.outputFormat = "png";

        const morpho::ConversionResult decoded = converter.convert(decodeRequest);
        if (!decoded.success || decoded.data.empty() || decoded.inputFormat != format ||
            !hasPngSignature(decoded.data)) {
            std::cerr << "Could not decode " << format << ": " << decoded.error << '\n';
            ok = false;
            continue;
        }

        std::cout << "Round-tripped " << format << " (" << encoded.data.size() << " bytes).\n";
    }

    std::filesystem::remove_all(directory, error);
    return ok ? 0 : 1;
}
