// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/image_converter.hpp"

#include "backends/image/codecs.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace morpho::image {
namespace {

std::string normalizeFormat(std::string format) {
    std::transform(format.begin(), format.end(), format.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return format;
}

std::string detectFormat(const std::filesystem::path &path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return {};
    }

    std::array<std::uint8_t, 32> header{};
    file.read(reinterpret_cast<char *>(header.data()), static_cast<std::streamsize>(header.size()));
    const std::size_t size = static_cast<std::size_t>(file.gcount());

    constexpr std::array<std::uint8_t, 8> kPng = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
    if (size >= kPng.size() && std::equal(kPng.begin(), kPng.end(), header.begin())) {
        return "png";
    }

    if (size >= 3 && header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF) {
        return "jpeg";
    }

    if (size >= 6 && (std::equal(header.begin(), header.begin() + 6, "GIF87a") ||
                      std::equal(header.begin(), header.begin() + 6, "GIF89a"))) {
        return "gif";
    }

    if (size >= 2 && header[0] == 'B' && header[1] == 'M') {
        return "bmp";
    }

    if (size >= 4 &&
        ((header[0] == 'I' && header[1] == 'I' && header[2] == 0x2A && header[3] == 0x00) ||
         (header[0] == 'M' && header[1] == 'M' && header[2] == 0x00 && header[3] == 0x2A))) {
        return "tiff";
    }

    if (size >= 12 && header[0] == 'R' && header[1] == 'I' && header[2] == 'F' &&
        header[3] == 'F' && header[8] == 'W' && header[9] == 'E' && header[10] == 'B' &&
        header[11] == 'P') {
        return "webp";
    }

    if (size >= 12 && header[4] == 'f' && header[5] == 't' && header[6] == 'y' &&
        header[7] == 'p') {
        for (std::size_t offset = 8; offset + 4 <= size; offset += 4) {
            if (header[offset] == 'a' && header[offset + 1] == 'v' && header[offset + 2] == 'i' &&
                (header[offset + 3] == 'f' || header[offset + 3] == 's')) {
                return "avif";
            }
        }
    }

    return {};
}

std::string extensionForFormat(const std::string &format) {
    if (format == "png") {
        return ".png";
    }
    if (format == "jpeg") {
        return ".jpg";
    }
    if (format == "webp") {
        return ".webp";
    }
    if (format == "avif") {
        return ".avif";
    }
    if (format == "tiff") {
        return ".tiff";
    }
    if (format == "bmp") {
        return ".bmp";
    }
    if (format == "gif") {
        return ".gif";
    }
    return {};
}

std::uint64_t inputFileSize(const std::filesystem::path &path) {
    std::error_code error;
    const std::uintmax_t size = std::filesystem::file_size(path, error);
    return error ? 0 : static_cast<std::uint64_t>(size);
}

ConversionResult failure(const ConversionRequest &request, std::string message) {
    ConversionResult result;
    result.inputPath = request.inputPath;
    result.inputBytes = inputFileSize(request.inputPath);
    result.error = std::move(message);
    return result;
}

bool decode(const std::string &format, const std::filesystem::path &path, Image &image,
            std::string &error) {
    if (format == "png") {
        return decodePng(path, image, error);
    }
    if (format == "jpeg") {
        return decodeJpeg(path, image, error);
    }
    if (format == "webp") {
        return decodeWebp(path, image, error);
    }
    if (format == "avif") {
        return decodeAvif(path, image, error);
    }
    if (format == "tiff") {
        return decodeTiff(path, image, error);
    }
    if (format == "bmp") {
        return decodeBmp(path, image, error);
    }
    if (format == "gif") {
        return decodeGif(path, image, error);
    }

    error = "Unsupported input image format.";
    return false;
}

bool encode(const std::string &format, const Image &image, std::vector<std::uint8_t> &output,
            std::string &error) {
    if (format == "png") {
        return encodePng(image, output, error);
    }
    if (format == "jpeg") {
        return encodeJpeg(image, output, error);
    }
    if (format == "webp") {
        return encodeWebp(image, output, error);
    }
    if (format == "avif") {
        return encodeAvif(image, output, error);
    }
    if (format == "tiff") {
        return encodeTiff(image, output, error);
    }
    if (format == "bmp") {
        return encodeBmp(image, output, error);
    }
    if (format == "gif") {
        return encodeGif(image, output, error);
    }

    error = "Unsupported output image format.";
    return false;
}

} // namespace

bool ImageConverter::supportsInput(const std::filesystem::path &path) const {
    return !detectFormat(path).empty();
}

ConversionResult ImageConverter::convert(const ConversionRequest &request) {
    const std::string inputFormat = detectFormat(request.inputPath);
    if (inputFormat.empty()) {
        return failure(request, "Unsupported or unreadable input image.");
    }

    const std::string outputFormat = normalizeFormat(request.outputFormat);
    if (extensionForFormat(outputFormat).empty()) {
        return failure(request, "Unsupported output image format.");
    }

    Image image;
    std::string error;
    if (!decode(inputFormat, request.inputPath, image, error)) {
        return failure(request, std::move(error));
    }

    std::vector<std::uint8_t> output;
    if (!encode(outputFormat, image, output, error)) {
        return failure(request, std::move(error));
    }

    std::filesystem::path outputName = request.inputPath.filename();
    outputName.replace_extension(extensionForFormat(outputFormat));

    ConversionResult result;
    result.success = true;
    result.inputPath = request.inputPath;
    result.inputFormat = inputFormat;
    result.outputFormat = outputFormat;
    result.outputFileName = outputName.string();
    result.inputBytes = inputFileSize(request.inputPath);
    result.data = std::move(output);
    return result;
}

} // namespace morpho::image
