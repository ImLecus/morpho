// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace morpho::image {

constexpr std::uint64_t kMaxPixels = 100'000'000;

struct Image {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgba;
};

bool validateImage(const Image &image, std::string &error);
std::vector<std::uint8_t> readFile(const std::filesystem::path &path, std::string &error);

} // namespace morpho::image
