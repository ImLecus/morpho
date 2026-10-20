// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <jpeglib.h>

namespace morpho::image {
namespace {

struct JpegErrorManager {
    jpeg_error_mgr base{};
    std::jmp_buf jumpBuffer{};
    char message[JMSG_LENGTH_MAX]{};
};

void jpegErrorExit(j_common_ptr info) {
    auto *error = reinterpret_cast<JpegErrorManager *>(info->err);
    (*info->err->format_message)(info, error->message);
    longjmp(error->jumpBuffer, 1);
}

void jpegOutputMessage(j_common_ptr) {}

std::string jpegMessage(const JpegErrorManager &error, const char *fallback) {
    return error.message[0] != '\0' ? std::string(error.message) : std::string(fallback);
}

struct JpegDecodeState {
    jpeg_decompress_struct info{};
    JpegErrorManager error{};
    FILE *file = nullptr;
    unsigned char *row = nullptr;
    unsigned char *rgb = nullptr;
    bool created = false;

    ~JpegDecodeState() {
        if (created) {
            jpeg_destroy_decompress(&info);
        }
        if (file != nullptr) {
            std::fclose(file);
        }
        std::free(row);
        std::free(rgb);
    }
};

struct JpegEncodeState {
    jpeg_compress_struct info{};
    JpegErrorManager error{};
    unsigned char *output = nullptr;
    unsigned long outputSize = 0;
    bool created = false;

    ~JpegEncodeState() {
        if (created) {
            jpeg_destroy_compress(&info);
        }
        std::free(output);
    }
};

} // namespace

bool decodeJpeg(const std::filesystem::path &path, Image &image, std::string &error) {
    JpegDecodeState state;
    state.file = std::fopen(path.c_str(), "rb");
    if (state.file == nullptr) {
        error = "Could not open the JPEG file.";
        return false;
    }

    state.info.err = jpeg_std_error(&state.error.base);
    state.error.base.error_exit = jpegErrorExit;
    state.error.base.output_message = jpegOutputMessage;

    if (setjmp(state.error.jumpBuffer) != 0) {
        error = jpegMessage(state.error, "Could not decode the JPEG file.");
        return false;
    }

    jpeg_create_decompress(&state.info);
    state.created = true;
    jpeg_stdio_src(&state.info, state.file);

    if (jpeg_read_header(&state.info, TRUE) != JPEG_HEADER_OK) {
        error = "The input is not a readable JPEG file.";
        return false;
    }

    const auto headerWidth = static_cast<std::uint64_t>(state.info.image_width);
    const auto headerHeight = static_cast<std::uint64_t>(state.info.image_height);
    if (headerWidth == 0 || headerHeight == 0 || headerWidth > 65'535 || headerHeight > 65'535 ||
        headerWidth * headerHeight > kMaxPixels) {
        error = "The JPEG dimensions exceed the supported limit.";
        return false;
    }

    if (state.info.jpeg_color_space == JCS_CMYK || state.info.jpeg_color_space == JCS_YCCK) {
        error = "CMYK JPEG images are not supported.";
        return false;
    }

    if (state.info.jpeg_color_space == JCS_GRAYSCALE) {
        state.info.out_color_space = JCS_GRAYSCALE;
    } else if (state.info.jpeg_color_space == JCS_RGB || state.info.jpeg_color_space == JCS_YCbCr) {
        state.info.out_color_space = JCS_RGB;
    } else {
        error = "The JPEG uses an unsupported color space.";
        return false;
    }

    if (!jpeg_start_decompress(&state.info)) {
        error = "Could not start JPEG decoding.";
        return false;
    }

    const auto width = static_cast<std::size_t>(state.info.output_width);
    const auto height = static_cast<std::size_t>(state.info.output_height);
    const int components = state.info.output_components;
    if (width == 0 || height == 0 || width > 65'535 || height > 65'535 ||
        width * height > kMaxPixels || (components != 1 && components != 3)) {
        error = "The JPEG has an unsupported layout.";
        return false;
    }

    const std::size_t rowBytes = width * static_cast<std::size_t>(components);
    const std::size_t rgbBytes = width * height * 3;
    state.row = static_cast<unsigned char *>(std::malloc(rowBytes));
    state.rgb = static_cast<unsigned char *>(std::malloc(rgbBytes));
    if (state.row == nullptr || state.rgb == nullptr) {
        error = "Not enough memory for the JPEG image.";
        return false;
    }

    std::size_t destination = 0;
    while (state.info.output_scanline < state.info.output_height) {
        JSAMPROW rowPointer = state.row;
        if (jpeg_read_scanlines(&state.info, &rowPointer, 1) != 1) {
            error = "Could not read a row from the JPEG.";
            return false;
        }

        if (components == 3) {
            std::memcpy(state.rgb + destination, state.row, width * 3);
            destination += width * 3;
        } else {
            for (std::size_t x = 0; x < width; ++x) {
                const unsigned char gray = state.row[x];
                state.rgb[destination++] = gray;
                state.rgb[destination++] = gray;
                state.rgb[destination++] = gray;
            }
        }
    }

    if (!jpeg_finish_decompress(&state.info)) {
        error = "Could not finish JPEG decoding.";
        return false;
    }

    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.rgba.resize(width * height * 4);
    for (std::size_t i = 0; i < width * height; ++i) {
        image.rgba[i * 4] = state.rgb[i * 3];
        image.rgba[i * 4 + 1] = state.rgb[i * 3 + 1];
        image.rgba[i * 4 + 2] = state.rgb[i * 3 + 2];
        image.rgba[i * 4 + 3] = 255;
    }

    return validateImage(image, error);
}

bool encodeJpeg(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    const std::size_t width = static_cast<std::size_t>(image.width);
    const std::size_t height = static_cast<std::size_t>(image.height);
    std::vector<unsigned char> rgb(width * height * 3);

    for (std::size_t i = 0; i < width * height; ++i) {
        const std::uint8_t alpha = image.rgba[i * 4 + 3];
        const std::uint32_t inverse = 255U - alpha;
        rgb[i * 3] =
            static_cast<unsigned char>((image.rgba[i * 4] * alpha + 255U * inverse + 127U) / 255U);
        rgb[i * 3 + 1] = static_cast<unsigned char>(
            (image.rgba[i * 4 + 1] * alpha + 255U * inverse + 127U) / 255U);
        rgb[i * 3 + 2] = static_cast<unsigned char>(
            (image.rgba[i * 4 + 2] * alpha + 255U * inverse + 127U) / 255U);
    }

    JpegEncodeState state;
    state.info.err = jpeg_std_error(&state.error.base);
    state.error.base.error_exit = jpegErrorExit;
    state.error.base.output_message = jpegOutputMessage;

    if (setjmp(state.error.jumpBuffer) != 0) {
        error = jpegMessage(state.error, "Could not encode the JPEG file.");
        return false;
    }

    jpeg_create_compress(&state.info);
    state.created = true;
    jpeg_mem_dest(&state.info, &state.output, &state.outputSize);

    state.info.image_width = static_cast<JDIMENSION>(width);
    state.info.image_height = static_cast<JDIMENSION>(height);
    state.info.input_components = 3;
    state.info.in_color_space = JCS_RGB;
    jpeg_set_defaults(&state.info);
    jpeg_set_quality(&state.info, 90, TRUE);
    jpeg_start_compress(&state.info, TRUE);

    while (state.info.next_scanline < state.info.image_height) {
        JSAMPROW rowPointer =
            rgb.data() + static_cast<std::size_t>(state.info.next_scanline) * width * 3;
        jpeg_write_scanlines(&state.info, &rowPointer, 1);
    }

    jpeg_finish_compress(&state.info);
    output.assign(state.output, state.output + state.outputSize);
    return true;
}

} // namespace morpho::image
