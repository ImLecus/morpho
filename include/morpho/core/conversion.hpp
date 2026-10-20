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
    std::string outputFormat;
};

struct ConversionResult {
    bool success = false;
    std::filesystem::path inputPath;
    std::string inputFormat;
    std::string outputFormat;
    std::string outputFileName;
    std::uint64_t inputBytes = 0;
    std::vector<std::uint8_t> data;
    std::string error;
};

class ConversionService {
public:
    virtual ~ConversionService() = default;

    virtual bool supportsInput(const std::filesystem::path &path) const = 0;
    virtual ConversionResult convert(const ConversionRequest &request) = 0;
};

} // namespace morpho
