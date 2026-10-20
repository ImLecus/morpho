// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <morpho/core/conversion.hpp>

namespace morpho::image {

class JpegToPngConverter final : public ConversionService {
public:
    ConversionResult convert(const ConversionRequest &request) override;
};

} // namespace morpho::image
