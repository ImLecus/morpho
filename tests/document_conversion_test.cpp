// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/morpho_converter.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <QGuiApplication>
#include <QString>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: morpho_document_conversion_test <input.md> <input.docx> <input.odt>\n";
        return 2;
    }

    QGuiApplication application(argc, argv);

    const std::filesystem::path markdown = argv[1];
    const std::filesystem::path docx = argv[2];
    const std::filesystem::path odt = argv[3];

    morpho::MorphoConverter converter;
    bool ok = true;

    morpho::ConversionRequest markdownRequest;
    markdownRequest.inputPath = markdown;
    markdownRequest.outputFormat = "pdf";
    const morpho::ConversionResult markdownResult = converter.convert(markdownRequest);
    if (!markdownResult.success || markdownResult.data.size() < 4 ||
        std::string(markdownResult.data.begin(), markdownResult.data.begin() + 4) != "%PDF" ||
        markdownResult.outputFileName != "tiny.pdf") {
        std::cerr << "Markdown to PDF failed: " << markdownResult.error << '\n';
        ok = false;
    } else {
        std::cout << "Converted Markdown to PDF (" << markdownResult.data.size() << " bytes).\n";
    }

    if (converter.supportsInput(docx)) {
        // The UI converts on a worker thread, so exercise that path as well.
        std::thread officeWorker([&converter, &docx, &odt, &ok]() {
            morpho::ConversionRequest docxRequest;
            docxRequest.inputPath = docx;
            docxRequest.outputFormat = "pdf";
            const morpho::ConversionResult docxResult = converter.convert(docxRequest);
            if (!docxResult.success || docxResult.data.size() < 4 ||
                std::string(docxResult.data.begin(), docxResult.data.begin() + 4) != "%PDF") {
                std::cerr << "DOCX to PDF failed: " << docxResult.error << '\n';
                ok = false;
            } else {
                std::cout << "Converted DOCX to PDF (" << docxResult.data.size() << " bytes).\n";
            }

            morpho::ConversionRequest odtRequest;
            odtRequest.inputPath = odt;
            odtRequest.outputFormat = "docx";
            const morpho::ConversionResult odtResult = converter.convert(odtRequest);
            if (!odtResult.success || odtResult.data.size() < 2 || odtResult.data[0] != 'P' ||
                odtResult.data[1] != 'K') {
                std::cerr << "ODT to DOCX failed: " << odtResult.error << '\n';
                ok = false;
            } else {
                std::cout << "Converted ODT to DOCX (" << odtResult.data.size() << " bytes).\n";
            }
        });
        officeWorker.join();
    } else {
        std::cout << "LibreOfficeKit is not available; skipping office document checks.\n";
    }

    if (converter.usedLibreOffice()) {
        // LibreOfficeKit 24.2 crashes in its shutdown path on some systems.
        std::fflush(nullptr);
        std::_Exit(ok ? 0 : 1);
    }

    return ok ? 0 : 1;
}
