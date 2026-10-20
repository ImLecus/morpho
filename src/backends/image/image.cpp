// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/image.hpp"

#include <cstdio>
#include <fstream>
#include <limits>
#include <system_error>

namespace morpho::image {

bool validateImage(const Image &image, std::string &error) {
    if (image.width <= 0 || image.height <= 0) {
        error = "The image has invalid dimensions.";
        return false;
    }

    const auto width = static_cast<std::uint64_t>(image.width);
    const auto height = static_cast<std::uint64_t>(image.height);
    if (width > 65'535 || height > 65'535 || width * height > kMaxPixels) {
        error = "The image dimensions exceed the supported limit.";
        return false;
    }

    if (image.rgba.size() != static_cast<std::size_t>(width * height * 4)) {
        error = "The decoded image buffer has an invalid size.";
        return false;
    }

    return true;
}

std::vector<std::uint8_t> readFile(const std::filesystem::path &path, std::string &error) {
    std::error_code sizeError;
    const std::uintmax_t size = std::filesystem::file_size(path, sizeError);
    if (sizeError) {
        error = "Could not read the input file size.";
        return {};
    }

    if (size > static_cast<std::uintmax_t>(std::numeric_limits<std::size_t>::max())) {
        error = "The input file is too large.";
        return {};
    }

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Could not open the input file.";
        return {};
    }

    std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
    if (!data.empty()) {
        file.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    if (!file) {
        error = "Could not read the input file.";
        return {};
    }

    return data;
}

} // namespace morpho::image
