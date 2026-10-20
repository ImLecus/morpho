// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <cstring>
#include <string>
#include <vector>

#include <webp/decode.h>
#include <webp/encode.h>

namespace morpho::image {

bool decodeWebp(const std::filesystem::path &path, Image &image, std::string &error) {
    const std::vector<std::uint8_t> data = readFile(path, error);
    if (data.empty()) {
        return false;
    }

    int width = 0;
    int height = 0;
    if (WebPGetInfo(data.data(), data.size(), &width, &height) == 0) {
        error = "The input is not a readable WebP file.";
        return false;
    }

    if (width <= 0 || height <= 0 || static_cast<std::uint64_t>(width) > 65'535 ||
        static_cast<std::uint64_t>(height) > 65'535 ||
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > kMaxPixels) {
        error = "The WebP dimensions exceed the supported limit.";
        return false;
    }

    std::uint8_t *pixels = WebPDecodeRGBA(data.data(), data.size(), &width, &height);
    if (pixels == nullptr) {
        error = "Could not decode the WebP file.";
        return false;
    }

    image.width = width;
    image.height = height;
    image.rgba.assign(pixels, pixels + static_cast<std::size_t>(width) *
                                           static_cast<std::size_t>(height) * 4);
    WebPFree(pixels);

    return validateImage(image, error);
}

bool encodeWebp(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    std::uint8_t *encoded = nullptr;
    const std::size_t size = WebPEncodeRGBA(image.rgba.data(), image.width, image.height,
                                            image.width * 4, 90.0F, &encoded);
    if (size == 0 || encoded == nullptr) {
        error = "Could not encode the WebP file.";
        return false;
    }

    output.assign(encoded, encoded + size);
    WebPFree(encoded);
    return true;
}

} // namespace morpho::image
