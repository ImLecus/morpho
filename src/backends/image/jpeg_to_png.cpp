// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/jpeg_to_png.hpp"

#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>
#include <utility>

#include <jpeglib.h>
#include <png.h>

namespace morpho::image {
namespace {

constexpr std::uint64_t kMaxDimension = 65'535;
constexpr std::uint64_t kMaxPixels = 100'000'000;

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

struct JpegState {
    jpeg_decompress_struct info{};
    JpegErrorManager error{};
    FILE *file = nullptr;
    unsigned char *row = nullptr;
    unsigned char *rgb = nullptr;
    bool created = false;

    ~JpegState() {
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

ConversionResult failure(const ConversionRequest &request, std::string message) {
    ConversionResult result;
    result.inputPath = request.inputPath;
    result.outputPath = request.outputPath;
    result.error = std::move(message);
    return result;
}

} // namespace

ConversionResult JpegToPngConverter::convert(const ConversionRequest &request) {
    JpegState state;

    state.file = std::fopen(request.inputPath.c_str(), "rb");
    if (state.file == nullptr) {
        return failure(request, "Could not open the input JPEG file.");
    }

    state.info.err = jpeg_std_error(&state.error.base);
    state.error.base.error_exit = jpegErrorExit;
    state.error.base.output_message = jpegOutputMessage;

    if (setjmp(state.error.jumpBuffer) != 0) {
        return failure(request, state.error.message);
    }

    jpeg_create_decompress(&state.info);
    state.created = true;
    jpeg_stdio_src(&state.info, state.file);

    if (jpeg_read_header(&state.info, TRUE) != JPEG_HEADER_OK) {
        return failure(request, "The input is not a readable JPEG file.");
    }

    const auto headerWidth = static_cast<std::uint64_t>(state.info.image_width);
    const auto headerHeight = static_cast<std::uint64_t>(state.info.image_height);
    if (headerWidth == 0 || headerHeight == 0 || headerWidth > kMaxDimension ||
        headerHeight > kMaxDimension || headerWidth * headerHeight > kMaxPixels) {
        return failure(request, "The JPEG dimensions exceed the supported limit.");
    }

    if (state.info.jpeg_color_space == JCS_CMYK || state.info.jpeg_color_space == JCS_YCCK) {
        return failure(request, "CMYK JPEG images are not supported yet.");
    }

    if (state.info.jpeg_color_space == JCS_GRAYSCALE) {
        state.info.out_color_space = JCS_GRAYSCALE;
    } else if (state.info.jpeg_color_space == JCS_RGB || state.info.jpeg_color_space == JCS_YCbCr) {
        state.info.out_color_space = JCS_RGB;
    } else {
        return failure(request, "The JPEG uses an unsupported color space.");
    }

    if (!jpeg_start_decompress(&state.info)) {
        return failure(request, "Could not start JPEG decoding.");
    }

    const auto width = static_cast<std::uint64_t>(state.info.output_width);
    const auto height = static_cast<std::uint64_t>(state.info.output_height);
    const int components = state.info.output_components;

    if (width == 0 || height == 0 || width > kMaxDimension || height > kMaxDimension ||
        width * height > kMaxPixels) {
        return failure(request, "The JPEG dimensions exceed the supported limit.");
    }

    if (components != 1 && components != 3) {
        return failure(request, "The JPEG has an unsupported color layout.");
    }

    const auto widthSize = static_cast<std::size_t>(width);
    const auto heightSize = static_cast<std::size_t>(height);
    const std::size_t rowBytes = widthSize * static_cast<std::size_t>(components);
    const std::size_t rgbBytes = widthSize * heightSize * 3;

    state.row = static_cast<unsigned char *>(std::malloc(rowBytes));
    state.rgb = static_cast<unsigned char *>(std::malloc(rgbBytes));
    if (state.row == nullptr || state.rgb == nullptr) {
        return failure(request, "Not enough memory for the image.");
    }

    std::size_t destination = 0;
    while (state.info.output_scanline < state.info.output_height) {
        JSAMPROW rowPointer = state.row;
        if (jpeg_read_scanlines(&state.info, &rowPointer, 1) != 1) {
            return failure(request, "Could not read a row from the JPEG.");
        }

        if (components == 3) {
            std::memcpy(state.rgb + destination, state.row, widthSize * 3);
            destination += widthSize * 3;
        } else {
            for (std::size_t x = 0; x < widthSize; ++x) {
                const unsigned char gray = state.row[x];
                state.rgb[destination++] = gray;
                state.rgb[destination++] = gray;
                state.rgb[destination++] = gray;
            }
        }
    }

    if (!jpeg_finish_decompress(&state.info)) {
        return failure(request, "Could not finish JPEG decoding.");
    }

    png_image image{};
    image.version = PNG_IMAGE_VERSION;
    image.width = static_cast<png_uint_32>(width);
    image.height = static_cast<png_uint_32>(height);
    image.format = PNG_FORMAT_RGB;

    std::filesystem::path temporary = request.outputPath;
    temporary += ".part";
    const std::string temporaryName = temporary.string();

    if (png_image_write_to_file(&image, temporaryName.c_str(), 0, state.rgb, 0, nullptr) == 0) {
        std::error_code removeError;
        std::filesystem::remove(temporary, removeError);
        const std::string message =
            image.message[0] != '\0' ? std::string(image.message) : "Could not write the PNG file.";
        return failure(request, message);
    }

    std::error_code renameError;
    std::filesystem::rename(temporary, request.outputPath, renameError);
    if (renameError) {
        std::error_code removeError;
        std::filesystem::remove(temporary, removeError);
        return failure(request, "Could not publish the PNG file: " + renameError.message());
    }

    ConversionResult result;
    result.success = true;
    result.inputPath = request.inputPath;
    result.outputPath = request.outputPath;
    return result;
}

} // namespace morpho::image
