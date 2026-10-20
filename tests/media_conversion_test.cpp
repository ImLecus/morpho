// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/media/media_converter.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {

bool writeFile(const std::filesystem::path &path, const std::vector<std::uint8_t> &data) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    file.write(reinterpret_cast<const char *>(data.data()),
               static_cast<std::streamsize>(data.size()));
    return static_cast<bool>(file);
}

} // namespace

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: morpho_media_conversion_test <input.mp4>\n";
        return 2;
    }

    const std::filesystem::path fixture = argv[1];
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() / "morpho-media-conversion-test";

    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr << "Could not create the test directory.\n";
        return 1;
    }

    morpho::media::MediaConverter converter;
    if (!converter.supportsInput(fixture)) {
        std::cerr << "The media input was not recognized.\n";
        return 1;
    }

    const std::vector<std::string> audioFormats = {"mp3", "m4a", "opus", "vorbis", "flac", "wav"};
    const std::vector<std::string> videoFormats = {"mp4", "webm", "mkv", "mov", "gif"};

    bool ok = true;
    for (const std::string &format : audioFormats) {
        morpho::ConversionRequest request;
        request.inputPath = fixture;
        request.outputFormat = format;

        const morpho::ConversionResult encoded = converter.convert(request);
        if (!encoded.success || encoded.data.empty() || encoded.outputFileName.empty()) {
            std::cerr << "Could not encode audio " << format << ": " << encoded.error << '\n';
            ok = false;
            continue;
        }

        const std::filesystem::path encodedPath = directory / encoded.outputFileName;
        if (!writeFile(encodedPath, encoded.data)) {
            std::cerr << "Could not write the audio test file.\n";
            ok = false;
            continue;
        }

        morpho::ConversionRequest decodeRequest;
        decodeRequest.inputPath = encodedPath;
        decodeRequest.outputFormat = "wav";
        const morpho::ConversionResult decoded = converter.convert(decodeRequest);
        if (!decoded.success || decoded.data.empty()) {
            std::cerr << "Could not decode audio " << format << ": " << decoded.error << '\n';
            ok = false;
            continue;
        }

        std::cout << "Converted audio to " << format << " (" << encoded.data.size() << " bytes).\n";
    }

    for (const std::string &format : videoFormats) {
        morpho::ConversionRequest request;
        request.inputPath = fixture;
        request.outputFormat = format;

        const morpho::ConversionResult encoded = converter.convert(request);
        if (!encoded.success || encoded.data.empty() || encoded.outputFileName.empty()) {
            std::cerr << "Could not encode video " << format << ": " << encoded.error << '\n';
            ok = false;
            continue;
        }

        const std::filesystem::path encodedPath = directory / encoded.outputFileName;
        if (!writeFile(encodedPath, encoded.data)) {
            std::cerr << "Could not write the video test file.\n";
            ok = false;
            continue;
        }

        morpho::ConversionRequest decodeRequest;
        decodeRequest.inputPath = encodedPath;
        decodeRequest.outputFormat = "mp4";
        const morpho::ConversionResult decoded = converter.convert(decodeRequest);
        if (!decoded.success || decoded.data.empty()) {
            std::cerr << "Could not decode video " << format << ": " << decoded.error << '\n';
            ok = false;
            continue;
        }

        std::cout << "Converted video to " << format << " (" << encoded.data.size() << " bytes).\n";
    }

    std::filesystem::remove_all(directory, error);
    return ok ? 0 : 1;
}
