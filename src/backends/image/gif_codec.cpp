// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <gif_lib.h>

namespace morpho::image {
namespace {

struct GifReader {
    const std::uint8_t *data = nullptr;
    std::size_t size = 0;
    std::size_t offset = 0;
};

int readGifBytes(GifFileType *gif, GifByteType *buffer, int count) {
    auto *reader = static_cast<GifReader *>(gif->UserData);
    if (reader == nullptr || count <= 0) {
        return 0;
    }

    const std::size_t remaining = reader->size - reader->offset;
    const std::size_t requested = static_cast<std::size_t>(count);
    const std::size_t copied = std::min(remaining, requested);
    if (copied > 0) {
        std::memcpy(buffer, reader->data + reader->offset, copied);
        reader->offset += copied;
    }
    return static_cast<int>(copied);
}

struct GifWriter {
    std::vector<std::uint8_t> data;
};

int writeGif(GifFileType *gif, const GifByteType *buffer, int count) {
    auto *writer = static_cast<GifWriter *>(gif->UserData);
    if (writer == nullptr || count <= 0) {
        return 0;
    }
    writer->data.insert(writer->data.end(), buffer, buffer + count);
    return count;
}

ColorMapObject *frameColorMap(const GifFileType *gif, const SavedImage &frame) {
    return frame.ImageDesc.ColorMap != nullptr ? frame.ImageDesc.ColorMap : gif->SColorMap;
}

int transparentIndex(const SavedImage &frame) {
    for (int i = 0; i < frame.ExtensionBlockCount; ++i) {
        const ExtensionBlock &extension = frame.ExtensionBlocks[i];
        if (extension.Function != GRAPHICS_EXT_FUNC_CODE || extension.ByteCount < 4) {
            continue;
        }

        GraphicsControlBlock control{};
        if (DGifExtensionToGCB(static_cast<std::size_t>(extension.ByteCount), extension.Bytes,
                               &control) == GIF_OK) {
            return control.TransparentColor;
        }
    }
    return NO_TRANSPARENT_COLOR;
}

} // namespace

bool decodeGif(const std::filesystem::path &path, Image &image, std::string &error) {
    const std::vector<std::uint8_t> data = readFile(path, error);
    if (data.empty()) {
        return false;
    }

    GifReader reader{data.data(), data.size(), 0};
    int gifErrorCode = 0;
    GifFileType *gif = DGifOpen(&reader, readGifBytes, &gifErrorCode);
    if (gif == nullptr) {
        error = "Could not open the GIF file.";
        return false;
    }

    if (DGifSlurp(gif) != GIF_OK || gif->ImageCount <= 0) {
        error = "Could not decode the GIF file.";
        DGifCloseFile(gif, &gifErrorCode);
        return false;
    }

    const SavedImage &frame = gif->SavedImages[0];
    const ColorMapObject *colors = frameColorMap(gif, frame);
    if (colors == nullptr || colors->ColorCount <= 0) {
        error = "The GIF has no color table.";
        DGifCloseFile(gif, &gifErrorCode);
        return false;
    }

    const int canvasWidth = gif->SWidth;
    const int canvasHeight = gif->SHeight;
    if (canvasWidth <= 0 || canvasHeight <= 0 || canvasWidth > 65'535 || canvasHeight > 65'535 ||
        static_cast<std::uint64_t>(canvasWidth) * static_cast<std::uint64_t>(canvasHeight) >
            kMaxPixels) {
        error = "The GIF dimensions exceed the supported limit.";
        DGifCloseFile(gif, &gifErrorCode);
        return false;
    }

    const int transparent = transparentIndex(frame);
    image.width = canvasWidth;
    image.height = canvasHeight;
    image.rgba.assign(
        static_cast<std::size_t>(canvasWidth) * static_cast<std::size_t>(canvasHeight) * 4, 0);

    const int left = frame.ImageDesc.Left;
    const int top = frame.ImageDesc.Top;
    const int frameWidth = frame.ImageDesc.Width;
    const int frameHeight = frame.ImageDesc.Height;

    for (int y = 0; y < frameHeight; ++y) {
        for (int x = 0; x < frameWidth; ++x) {
            const int canvasX = left + x;
            const int canvasY = top + y;
            if (canvasX < 0 || canvasY < 0 || canvasX >= canvasWidth || canvasY >= canvasHeight) {
                continue;
            }

            const std::size_t source =
                static_cast<std::size_t>(y) * static_cast<std::size_t>(frameWidth) +
                static_cast<std::size_t>(x);
            if (source >=
                static_cast<std::size_t>(frameWidth) * static_cast<std::size_t>(frameHeight)) {
                continue;
            }

            const int index = frame.RasterBits[source];
            if (index == transparent || index < 0 || index >= colors->ColorCount) {
                continue;
            }

            const GifColorType &color = colors->Colors[index];
            const std::size_t destination =
                (static_cast<std::size_t>(canvasY) * static_cast<std::size_t>(canvasWidth) +
                 static_cast<std::size_t>(canvasX)) *
                4;
            image.rgba[destination] = color.Red;
            image.rgba[destination + 1] = color.Green;
            image.rgba[destination + 2] = color.Blue;
            image.rgba[destination + 3] = 255;
        }
    }

    DGifCloseFile(gif, &gifErrorCode);
    return validateImage(image, error);
}

bool encodeGif(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    GifColorType palette[256]{};
    palette[0] = GifColorType{0, 0, 0};
    for (int red = 0; red < 6; ++red) {
        for (int green = 0; green < 6; ++green) {
            for (int blue = 0; blue < 6; ++blue) {
                const int index = 1 + red * 36 + green * 6 + blue;
                palette[index] = GifColorType{static_cast<GifByteType>(red * 51),
                                              static_cast<GifByteType>(green * 51),
                                              static_cast<GifByteType>(blue * 51)};
            }
        }
    }

    ColorMapObject *colorMap = GifMakeMapObject(256, palette);
    if (colorMap == nullptr) {
        error = "Could not create the GIF palette.";
        return false;
    }

    GifWriter writer;
    int gifErrorCode = 0;
    GifFileType *gif = EGifOpen(&writer, writeGif, &gifErrorCode);
    if (gif == nullptr) {
        GifFreeMapObject(colorMap);
        error = "Could not create the GIF encoder.";
        return false;
    }

    if (EGifPutScreenDesc(gif, image.width, image.height, 8, 0, colorMap) != GIF_OK) {
        error = "Could not write the GIF header.";
        EGifCloseFile(gif, &gifErrorCode);
        GifFreeMapObject(colorMap);
        return false;
    }

    GraphicsControlBlock control{};
    control.DisposalMode = DISPOSE_DO_NOT;
    control.UserInputFlag = false;
    control.DelayTime = 0;
    control.TransparentColor = 0;

    GifByteType controlExtension[4]{};
    const std::size_t controlSize = EGifGCBToExtension(&control, controlExtension);
    if (EGifPutExtension(gif, GRAPHICS_EXT_FUNC_CODE, static_cast<int>(controlSize),
                         controlExtension) != GIF_OK) {
        error = "Could not write the GIF transparency data.";
        EGifCloseFile(gif, &gifErrorCode);
        GifFreeMapObject(colorMap);
        return false;
    }

    if (EGifPutImageDesc(gif, 0, 0, image.width, image.height, false, nullptr) != GIF_OK) {
        error = "Could not write the GIF image descriptor.";
        EGifCloseFile(gif, &gifErrorCode);
        GifFreeMapObject(colorMap);
        return false;
    }

    std::vector<GifPixelType> row(static_cast<std::size_t>(image.width));
    for (int y = 0; y < image.height; ++y) {
        for (int x = 0; x < image.width; ++x) {
            const std::size_t source =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width) +
                 static_cast<std::size_t>(x)) *
                4;
            const std::uint8_t alpha = image.rgba[source + 3];
            if (alpha < 128) {
                row[static_cast<std::size_t>(x)] = 0;
                continue;
            }

            const int red = (static_cast<int>(image.rgba[source]) * 5 + 127) / 255;
            const int green = (static_cast<int>(image.rgba[source + 1]) * 5 + 127) / 255;
            const int blue = (static_cast<int>(image.rgba[source + 2]) * 5 + 127) / 255;
            row[static_cast<std::size_t>(x)] =
                static_cast<GifPixelType>(1 + red * 36 + green * 6 + blue);
        }

        if (EGifPutLine(gif, row.data(), image.width) != GIF_OK) {
            error = "Could not write a GIF row.";
            EGifCloseFile(gif, &gifErrorCode);
            GifFreeMapObject(colorMap);
            return false;
        }
    }

    if (EGifCloseFile(gif, &gifErrorCode) != GIF_OK) {
        GifFreeMapObject(colorMap);
        error = "Could not finish the GIF file.";
        return false;
    }

    GifFreeMapObject(colorMap);
    output = std::move(writer.data);
    return true;
}

} // namespace morpho::image
