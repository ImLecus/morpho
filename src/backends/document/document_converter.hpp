// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <morpho/core/conversion.hpp>

namespace morpho::document {

class DocumentConverter final : public ConversionService {
public:
    bool supportsInput(const std::filesystem::path &path) const override;
    ConversionResult convert(const ConversionRequest &request) override;

    /// True after the LibreOfficeKit office instance was initialized.
    /// The process must avoid LibreOfficeKit's shutdown path in that case.
    bool usedLibreOffice() const;
};

} // namespace morpho::document
