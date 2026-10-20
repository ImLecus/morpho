// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace morpho {

struct ConversionRequest {
    std::filesystem::path inputPath;
};

struct ConversionResult {
    bool success = false;
    std::filesystem::path inputPath;
    std::string outputFileName;
    std::vector<std::uint8_t> data;
    std::string error;
};

class ConversionService {
public:
    virtual ~ConversionService() = default;

    virtual ConversionResult convert(const ConversionRequest &request) = 0;
};

} // namespace morpho
