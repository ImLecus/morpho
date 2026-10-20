// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <filesystem>
#include <string>

namespace morpho {

struct ConversionRequest {
    std::filesystem::path inputPath;
    std::filesystem::path outputPath;
};

struct ConversionResult {
    bool success = false;
    std::filesystem::path inputPath;
    std::filesystem::path outputPath;
    std::string error;
};

class ConversionService {
public:
    virtual ~ConversionService() = default;

    virtual ConversionResult convert(const ConversionRequest &request) = 0;
};

} // namespace morpho
