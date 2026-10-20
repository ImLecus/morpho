// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <png.h>

namespace morpho::image {
namespace {

std::string pngMessage(const png_image &image, const char *fallback) {
    return image.message[0] != '\0' ? std::string(image.message) : std::string(fallback);
}

} // namespace

bool decodePng(const std::filesystem::path &path, Image &image, std::string &error) {
    png_image png{};
    png.version = PNG_IMAGE_VERSION;

    const std::string fileName = path.string();
    if (png_image_begin_read_from_file(&png, fileName.c_str()) == 0) {
        error = pngMessage(png, "Could not read the PNG file.");
        return false;
    }

    png.format = PNG_FORMAT_RGBA;
    const std::size_t size = PNG_IMAGE_SIZE(png);
    std::vector<std::uint8_t> pixels(size);

    if (png_image_finish_read(&png, nullptr, pixels.data(), 0, nullptr) == 0) {
        error = pngMessage(png, "Could not decode the PNG file.");
        png_image_free(&png);
        return false;
    }

    image.width = static_cast<int>(png.width);
    image.height = static_cast<int>(png.height);
    image.rgba = std::move(pixels);
    png_image_free(&png);

    if (!validateImage(image, error)) {
        return false;
    }

    return true;
}

bool encodePng(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    png_image png{};
    png.version = PNG_IMAGE_VERSION;
    png.width = static_cast<png_uint_32>(image.width);
    png.height = static_cast<png_uint_32>(image.height);
    png.format = PNG_FORMAT_RGBA;

    png_alloc_size_t size = 0;
    if (png_image_write_get_memory_size(png, size, 0, image.rgba.data(), 0, nullptr) == 0) {
        error = pngMessage(png, "Could not prepare the PNG encoder.");
        return false;
    }

    output.resize(static_cast<std::size_t>(size));
    if (png_image_write_to_memory(&png, output.data(), &size, 0, image.rgba.data(), 0, nullptr) ==
        0) {
        error = pngMessage(png, "Could not encode the PNG file.");
        return false;
    }

    output.resize(static_cast<std::size_t>(size));
    return true;
}

} // namespace morpho::image
