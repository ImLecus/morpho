// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <morpho/core/conversion.hpp>

namespace morpho::media {

class MediaConverter final : public ConversionService {
public:
    bool supportsInput(const std::filesystem::path &path) const override;
    ConversionResult convert(const ConversionRequest &request) override;
};

} // namespace morpho::media
