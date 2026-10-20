// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace morpho::image {
namespace {

constexpr std::uint32_t kFileHeaderSize = 14;
constexpr std::uint32_t kInfoHeaderSize = 40;
constexpr std::uint32_t kPixelOffset = kFileHeaderSize + kInfoHeaderSize;

std::uint16_t read16(const std::uint8_t *data) {
    return static_cast<std::uint16_t>(data[0]) |
           static_cast<std::uint16_t>(static_cast<std::uint16_t>(data[1]) << 8U);
}

std::uint32_t read32(const std::uint8_t *data) {
    return static_cast<std::uint32_t>(data[0]) | (static_cast<std::uint32_t>(data[1]) << 8U) |
           (static_cast<std::uint32_t>(data[2]) << 16U) |
           (static_cast<std::uint32_t>(data[3]) << 24U);
}

std::int32_t read32Signed(const std::uint8_t *data) {
    return static_cast<std::int32_t>(read32(data));
}

void append16(std::vector<std::uint8_t> &output, std::uint16_t value) {
    output.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
}

void append32(std::vector<std::uint8_t> &output, std::uint32_t value) {
    output.push_back(static_cast<std::uint8_t>(value & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    output.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

std::size_t rowStride(int width, int bitsPerPixel) {
    const std::uint64_t bits =
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(bitsPerPixel);
    return static_cast<std::size_t>(((bits + 31U) / 32U) * 4U);
}

} // namespace

bool decodeBmp(const std::filesystem::path &path, Image &image, std::string &error) {
    const std::vector<std::uint8_t> data = readFile(path, error);
    if (data.size() < kPixelOffset || data[0] != 'B' || data[1] != 'M') {
        error = "The input is not a readable BMP file.";
        return false;
    }

    const std::uint32_t pixelOffset = read32(data.data() + 10);
    const std::uint32_t headerSize = read32(data.data() + 14);
    if (headerSize < kInfoHeaderSize || pixelOffset > data.size()) {
        error = "The BMP header is not supported.";
        return false;
    }

    const std::int32_t signedWidth = read32Signed(data.data() + 18);
    const std::int32_t signedHeight = read32Signed(data.data() + 22);
    const std::uint16_t planes = read16(data.data() + 26);
    const std::uint16_t bitsPerPixel = read16(data.data() + 28);
    const std::uint32_t compression = read32(data.data() + 30);

    if (planes != 1 || signedWidth <= 0 || signedHeight == 0 || compression != 0) {
        error = "The BMP layout is not supported.";
        return false;
    }

    const bool topDown = signedHeight < 0;
    const int width = signedWidth;
    const int height = topDown ? -signedHeight : signedHeight;
    if (width <= 0 || height <= 0 || width > 65'535 || height > 65'535 ||
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > kMaxPixels) {
        error = "The BMP dimensions exceed the supported limit.";
        return false;
    }

    if (bitsPerPixel != 8 && bitsPerPixel != 24 && bitsPerPixel != 32) {
        error = "Only 8-bit, 24-bit, and 32-bit BMP files are supported.";
        return false;
    }

    std::vector<std::uint32_t> palette;
    if (bitsPerPixel == 8) {
        const std::uint32_t colorCount = read32(data.data() + 46);
        const std::uint32_t count = colorCount == 0 ? 256 : colorCount;
        const std::size_t paletteOffset = kFileHeaderSize + headerSize;
        if (count > 256 || paletteOffset + static_cast<std::size_t>(count) * 4 > data.size()) {
            error = "The BMP palette is invalid.";
            return false;
        }
        palette.resize(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const std::uint8_t *entry =
                data.data() + paletteOffset + static_cast<std::size_t>(i) * 4;
            palette[i] = (static_cast<std::uint32_t>(entry[2]) << 16U) |
                         (static_cast<std::uint32_t>(entry[1]) << 8U) |
                         static_cast<std::uint32_t>(entry[0]);
        }
    }

    const std::size_t stride = rowStride(width, bitsPerPixel);
    if (pixelOffset + stride * static_cast<std::size_t>(height) > data.size()) {
        error = "The BMP pixel data is truncated.";
        return false;
    }

    image.width = width;
    image.height = height;
    image.rgba.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
    bool hasAlpha = false;

    for (int y = 0; y < height; ++y) {
        const int sourceRow = topDown ? y : height - 1 - y;
        const std::uint8_t *row =
            data.data() + pixelOffset + stride * static_cast<std::size_t>(sourceRow);
        for (int x = 0; x < width; ++x) {
            std::uint8_t red = 0;
            std::uint8_t green = 0;
            std::uint8_t blue = 0;
            std::uint8_t alpha = 255;

            if (bitsPerPixel == 8) {
                const std::uint8_t index = row[x];
                if (index >= palette.size()) {
                    error = "The BMP palette index is invalid.";
                    return false;
                }
                const std::uint32_t color = palette[index];
                red = static_cast<std::uint8_t>((color >> 16U) & 0xFFU);
                green = static_cast<std::uint8_t>((color >> 8U) & 0xFFU);
                blue = static_cast<std::uint8_t>(color & 0xFFU);
            } else if (bitsPerPixel == 24) {
                const std::uint8_t *pixel = row + static_cast<std::size_t>(x) * 3;
                blue = pixel[0];
                green = pixel[1];
                red = pixel[2];
            } else {
                const std::uint8_t *pixel = row + static_cast<std::size_t>(x) * 4;
                blue = pixel[0];
                green = pixel[1];
                red = pixel[2];
                alpha = pixel[3];
                hasAlpha = hasAlpha || alpha != 0;
            }

            const std::size_t destination =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                 static_cast<std::size_t>(x)) *
                4;
            image.rgba[destination] = red;
            image.rgba[destination + 1] = green;
            image.rgba[destination + 2] = blue;
            image.rgba[destination + 3] = alpha;
        }
    }

    if (bitsPerPixel == 32 && !hasAlpha) {
        for (std::size_t i = 3; i < image.rgba.size(); i += 4) {
            image.rgba[i] = 255;
        }
    }

    return validateImage(image, error);
}

bool encodeBmp(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    const int width = image.width;
    const int height = image.height;
    const std::size_t stride = rowStride(width, 24);
    const std::size_t pixelBytes = stride * static_cast<std::size_t>(height);
    const std::size_t fileSize = kPixelOffset + pixelBytes;

    output.clear();
    output.reserve(fileSize);
    output.push_back('B');
    output.push_back('M');
    append32(output, static_cast<std::uint32_t>(fileSize));
    append16(output, 0);
    append16(output, 0);
    append32(output, kPixelOffset);

    append32(output, kInfoHeaderSize);
    append32(output, static_cast<std::uint32_t>(width));
    append32(output, static_cast<std::uint32_t>(height));
    append16(output, 1);
    append16(output, 24);
    append32(output, 0);
    append32(output, static_cast<std::uint32_t>(pixelBytes));
    append32(output, 2835);
    append32(output, 2835);
    append32(output, 0);
    append32(output, 0);

    output.resize(fileSize, 0);
    for (int y = 0; y < height; ++y) {
        const int destinationRow = height - 1 - y;
        std::uint8_t *row =
            output.data() + kPixelOffset + stride * static_cast<std::size_t>(destinationRow);
        for (int x = 0; x < width; ++x) {
            const std::size_t source =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) +
                 static_cast<std::size_t>(x)) *
                4;
            const std::uint8_t alpha = image.rgba[source + 3];
            const std::uint32_t inverse = 255U - alpha;
            const std::uint8_t red = static_cast<std::uint8_t>(
                (image.rgba[source] * alpha + 255U * inverse + 127U) / 255U);
            const std::uint8_t green = static_cast<std::uint8_t>(
                (image.rgba[source + 1] * alpha + 255U * inverse + 127U) / 255U);
            const std::uint8_t blue = static_cast<std::uint8_t>(
                (image.rgba[source + 2] * alpha + 255U * inverse + 127U) / 255U);

            row[static_cast<std::size_t>(x) * 3] = blue;
            row[static_cast<std::size_t>(x) * 3 + 1] = green;
            row[static_cast<std::size_t>(x) * 3 + 2] = red;
        }
    }

    return true;
}

} // namespace morpho::image
