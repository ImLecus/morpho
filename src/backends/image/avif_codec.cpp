// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <cstring>
#include <string>
#include <vector>

#include <avif/avif.h>

namespace morpho::image {
namespace {

std::string avifMessage(const char *operation, avifResult result) {
    return std::string(operation) + ": " + avifResultToString(result);
}

} // namespace

bool decodeAvif(const std::filesystem::path &path, Image &image, std::string &error) {
    const std::vector<std::uint8_t> data = readFile(path, error);
    if (data.empty()) {
        return false;
    }

    avifDecoder *decoder = avifDecoderCreate();
    if (decoder == nullptr) {
        error = "Could not create the AVIF decoder.";
        return false;
    }

    decoder->maxThreads = 1;
    avifImage *avif = avifImageCreateEmpty();
    if (avif == nullptr) {
        avifDecoderDestroy(decoder);
        error = "Could not create the AVIF image.";
        return false;
    }

    const avifResult readResult = avifDecoderReadMemory(decoder, avif, data.data(), data.size());
    if (readResult != AVIF_RESULT_OK) {
        error = avifMessage("Could not decode the AVIF file", readResult);
        avifImageDestroy(avif);
        avifDecoderDestroy(decoder);
        return false;
    }

    if (avif->width == 0 || avif->height == 0 || avif->width > 65'535 || avif->height > 65'535 ||
        static_cast<std::uint64_t>(avif->width) * avif->height > kMaxPixels) {
        error = "The AVIF dimensions exceed the supported limit.";
        avifImageDestroy(avif);
        avifDecoderDestroy(decoder);
        return false;
    }

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, avif);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.depth = 8;
    rgb.maxThreads = 1;

    const avifResult allocateResult = avifRGBImageAllocatePixels(&rgb);
    if (allocateResult != AVIF_RESULT_OK) {
        error = avifMessage("Could not allocate the AVIF image", allocateResult);
        avifImageDestroy(avif);
        avifDecoderDestroy(decoder);
        return false;
    }

    const avifResult convertResult = avifImageYUVToRGB(avif, &rgb);
    if (convertResult != AVIF_RESULT_OK) {
        error = avifMessage("Could not convert the AVIF image", convertResult);
        avifRGBImageFreePixels(&rgb);
        avifImageDestroy(avif);
        avifDecoderDestroy(decoder);
        return false;
    }

    image.width = static_cast<int>(avif->width);
    image.height = static_cast<int>(avif->height);
    image.rgba.resize(static_cast<std::size_t>(image.width) *
                      static_cast<std::size_t>(image.height) * 4);

    const std::size_t rowBytes = static_cast<std::size_t>(image.width) * 4;
    for (int y = 0; y < image.height; ++y) {
        std::memcpy(image.rgba.data() + static_cast<std::size_t>(y) * rowBytes,
                    rgb.pixels + static_cast<std::size_t>(y) * rgb.rowBytes, rowBytes);
        if (avif->alphaPlane == nullptr) {
            for (int x = 0; x < image.width; ++x) {
                image.rgba[static_cast<std::size_t>(y) * rowBytes +
                           static_cast<std::size_t>(x) * 4 + 3] = 255;
            }
        }
    }

    avifRGBImageFreePixels(&rgb);
    avifImageDestroy(avif);
    avifDecoderDestroy(decoder);

    return validateImage(image, error);
}

bool encodeAvif(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    avifImage *avif =
        avifImageCreate(static_cast<std::uint32_t>(image.width),
                        static_cast<std::uint32_t>(image.height), 8, AVIF_PIXEL_FORMAT_YUV444);
    if (avif == nullptr) {
        error = "Could not create the AVIF image.";
        return false;
    }

    avifRGBImage rgb;
    avifRGBImageSetDefaults(&rgb, avif);
    rgb.format = AVIF_RGB_FORMAT_RGBA;
    rgb.depth = 8;
    rgb.pixels = const_cast<std::uint8_t *>(image.rgba.data());
    rgb.rowBytes = static_cast<std::uint32_t>(image.width) * 4;

    const avifResult convertResult = avifImageRGBToYUV(avif, &rgb);
    if (convertResult != AVIF_RESULT_OK) {
        error = avifMessage("Could not convert the image to AVIF", convertResult);
        avifImageDestroy(avif);
        return false;
    }

    avifEncoder *encoder = avifEncoderCreate();
    if (encoder == nullptr) {
        error = "Could not create the AVIF encoder.";
        avifImageDestroy(avif);
        return false;
    }

    encoder->maxThreads = 1;
    encoder->speed = 6;
    encoder->quality = 60;
    encoder->qualityAlpha = 60;

    avifRWData encoded = AVIF_DATA_EMPTY;
    const avifResult writeResult = avifEncoderWrite(encoder, avif, &encoded);
    if (writeResult != AVIF_RESULT_OK) {
        error = avifMessage("Could not encode the AVIF file", writeResult);
        avifRWDataFree(&encoded);
        avifEncoderDestroy(encoder);
        avifImageDestroy(avif);
        return false;
    }

    output.assign(encoded.data, encoded.data + encoded.size);
    avifRWDataFree(&encoded);
    avifEncoderDestroy(encoder);
    avifImageDestroy(avif);
    return true;
}

} // namespace morpho::image
