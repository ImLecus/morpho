// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include "backends/image/image.hpp"

namespace morpho::image {

bool decodePng(const std::filesystem::path &path, Image &image, std::string &error);
bool encodePng(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

bool decodeJpeg(const std::filesystem::path &path, Image &image, std::string &error);
bool encodeJpeg(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

bool decodeWebp(const std::filesystem::path &path, Image &image, std::string &error);
bool encodeWebp(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

bool decodeAvif(const std::filesystem::path &path, Image &image, std::string &error);
bool encodeAvif(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

bool decodeTiff(const std::filesystem::path &path, Image &image, std::string &error);
bool encodeTiff(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

bool decodeBmp(const std::filesystem::path &path, Image &image, std::string &error);
bool encodeBmp(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

bool decodeGif(const std::filesystem::path &path, Image &image, std::string &error);
bool encodeGif(const Image &image, std::vector<std::uint8_t> &output, std::string &error);

} // namespace morpho::image
