// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <morpho/core/conversion.hpp>

#include "backends/image/image_converter.hpp"
#include "backends/media/media_converter.hpp"

namespace morpho {

class MorphoConverter final : public ConversionService {
public:
    bool supportsInput(const std::filesystem::path &path) const override;
    ConversionResult convert(const ConversionRequest &request) override;

private:
    image::ImageConverter m_image;
    media::MediaConverter m_media;
};

} // namespace morpho
