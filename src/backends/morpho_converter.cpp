// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/morpho_converter.hpp"

namespace morpho {
namespace {

bool isImageOutput(const std::string &format) {
    return format == "png" || format == "jpeg" || format == "webp" || format == "avif" ||
           format == "tiff" || format == "bmp" || format == "gif";
}

bool isDocumentOutput(const std::string &format) {
    return format == "pdf" || format == "docx" || format == "odt";
}

} // namespace

bool MorphoConverter::supportsInput(const std::filesystem::path &path) const {
    return m_image.supportsInput(path) || m_media.supportsInput(path) ||
           m_document.supportsInput(path);
}

bool MorphoConverter::usedLibreOffice() const {
    return m_document.usedLibreOffice();
}

ConversionResult MorphoConverter::convert(const ConversionRequest &request) {
    if (isDocumentOutput(request.outputFormat) && m_document.supportsInput(request.inputPath)) {
        return m_document.convert(request);
    }
    if (isImageOutput(request.outputFormat) && m_image.supportsInput(request.inputPath)) {
        return m_image.convert(request);
    }
    return m_media.convert(request);
}

} // namespace morpho
