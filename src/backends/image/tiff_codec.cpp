// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/codecs.hpp"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include <tiffio.h>

namespace morpho::image {
namespace {

struct TiffWriter {
    std::vector<std::uint8_t> data;
    std::size_t offset = 0;
};

tsize_t tiffRead(thandle_t, tdata_t, tsize_t) {
    return 0;
}

tsize_t tiffWrite(thandle_t handle, tdata_t buffer, tsize_t size) {
    auto *writer = static_cast<TiffWriter *>(handle);
    if (size < 0) {
        return 0;
    }

    const auto count = static_cast<std::size_t>(size);
    if (writer->offset + count > writer->data.size()) {
        writer->data.resize(writer->offset + count);
    }
    std::memcpy(writer->data.data() + writer->offset, buffer, count);
    writer->offset += count;
    return size;
}

toff_t tiffSeek(thandle_t handle, toff_t offset, int whence) {
    auto *writer = static_cast<TiffWriter *>(handle);
    switch (whence) {
    case SEEK_SET:
        writer->offset = static_cast<std::size_t>(offset);
        break;
    case SEEK_CUR:
        writer->offset += static_cast<std::size_t>(offset);
        break;
    case SEEK_END:
        writer->offset = writer->data.size() + static_cast<std::size_t>(offset);
        break;
    default:
        return static_cast<toff_t>(-1);
    }
    return static_cast<toff_t>(writer->offset);
}

int tiffClose(thandle_t) {
    return 0;
}

toff_t tiffSize(thandle_t handle) {
    const auto *writer = static_cast<const TiffWriter *>(handle);
    return static_cast<toff_t>(writer->data.size());
}

int tiffMap(thandle_t, tdata_t *, toff_t *) {
    return 0;
}

void tiffUnmap(thandle_t, tdata_t, toff_t) {}

} // namespace

bool decodeTiff(const std::filesystem::path &path, Image &image, std::string &error) {
    TIFF *tiff = TIFFOpen(path.string().c_str(), "r");
    if (tiff == nullptr) {
        error = "Could not open the TIFF file.";
        return false;
    }

    std::uint32_t width = 0;
    std::uint32_t height = 0;
    if (TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width) != 1 ||
        TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height) != 1 || width == 0 || height == 0 ||
        width > 65'535 || height > 65'535 ||
        static_cast<std::uint64_t>(width) * height > kMaxPixels) {
        error = "The TIFF dimensions exceed the supported limit.";
        TIFFClose(tiff);
        return false;
    }

    std::vector<std::uint32_t> raster(static_cast<std::size_t>(width) * height);
    if (TIFFReadRGBAImageOriented(tiff, width, height, raster.data(), ORIENTATION_TOPLEFT, 0) ==
        0) {
        error = "Could not decode the TIFF file.";
        TIFFClose(tiff);
        return false;
    }

    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.rgba.resize(static_cast<std::size_t>(width) * height * 4);
    for (std::size_t i = 0; i < raster.size(); ++i) {
        const std::uint32_t pixel = raster[i];
        image.rgba[i * 4] = static_cast<std::uint8_t>(TIFFGetR(pixel));
        image.rgba[i * 4 + 1] = static_cast<std::uint8_t>(TIFFGetG(pixel));
        image.rgba[i * 4 + 2] = static_cast<std::uint8_t>(TIFFGetB(pixel));
        image.rgba[i * 4 + 3] = static_cast<std::uint8_t>(TIFFGetA(pixel));
    }

    TIFFClose(tiff);
    return validateImage(image, error);
}

bool encodeTiff(const Image &image, std::vector<std::uint8_t> &output, std::string &error) {
    if (!validateImage(image, error)) {
        return false;
    }

    TiffWriter writer;
    TIFF *tiff = TIFFClientOpen("morpho-memory", "w", &writer, tiffRead, tiffWrite, tiffSeek,
                                tiffClose, tiffSize, tiffMap, tiffUnmap);
    if (tiff == nullptr) {
        error = "Could not create the TIFF encoder.";
        return false;
    }

    const std::uint32_t width = static_cast<std::uint32_t>(image.width);
    const std::uint32_t height = static_cast<std::uint32_t>(image.height);
    std::uint16_t extraSamples = EXTRASAMPLE_UNASSALPHA;

    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);
    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4);
    TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8);
    TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extraSamples);
    TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_DEFLATE);
    TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, height);

    for (std::uint32_t row = 0; row < height; ++row) {
        auto *rowData = const_cast<std::uint8_t *>(image.rgba.data() +
                                                   static_cast<std::size_t>(row) * width * 4);
        if (TIFFWriteScanline(tiff, rowData, row, 0) < 0) {
            error = "Could not write the TIFF file.";
            TIFFClose(tiff);
            return false;
        }
    }

    TIFFClose(tiff);
    output = std::move(writer.data);
    return true;
}

} // namespace morpho::image
