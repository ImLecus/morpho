// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/document/document_converter.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTextDocument>
#include <QUrl>

#ifdef MORPHO_HAS_LIBREOFFICEKIT
#include <LibreOfficeKit/LibreOfficeKit.h>
#include <LibreOfficeKit/LibreOfficeKitInit.h>
#endif

namespace morpho::document {
namespace {

std::string lowerExtension(const std::filesystem::path &path) {
    std::string extension = path.extension().string();
    if (!extension.empty() && extension.front() == '.') {
        extension.erase(extension.begin());
    }
    std::transform(
        extension.begin(), extension.end(), extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return extension;
}

std::string upperExtension(const std::filesystem::path &path) {
    std::string extension = lowerExtension(path);
    std::transform(
        extension.begin(), extension.end(), extension.begin(),
        [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    return extension.empty() ? "DOCUMENT" : extension;
}

bool isMarkdown(const std::string &extension) {
    return extension == "md" || extension == "markdown";
}

bool isOfficeInput(const std::string &extension) {
    return extension == "docx" || extension == "doc" || extension == "odt" || extension == "rtf" ||
           extension == "txt";
}

QString fromPath(const std::filesystem::path &path) {
    const std::string bytes = path.string();
    return QFile::decodeName(QByteArray(bytes.data(), static_cast<int>(bytes.size())));
}

#ifdef MORPHO_HAS_LIBREOFFICEKIT
QString fileUrl(const std::filesystem::path &path) {
    std::error_code error;
    std::filesystem::path absolute = std::filesystem::absolute(path, error);
    if (error) {
        absolute = path;
    }
    return QUrl::fromLocalFile(fromPath(absolute)).toString(QUrl::FullyEncoded);
}
#endif

std::string extensionForOutput(const std::string &format) {
    if (format == "pdf") {
        return ".pdf";
    }
    if (format == "docx") {
        return ".docx";
    }
    if (format == "odt") {
        return ".odt";
    }
    return {};
}

ConversionResult failure(const ConversionRequest &request, std::string message) {
    ConversionResult result;
    result.inputPath = request.inputPath;
    result.inputFormat = upperExtension(request.inputPath);
    result.outputFormat = request.outputFormat;
    result.error = std::move(message);
    return result;
}

bool convertMarkdownToPdf(const std::filesystem::path &input, std::vector<std::uint8_t> &output,
                          std::string &error) {
    QFile file(fromPath(input));
    if (!file.open(QIODevice::ReadOnly)) {
        error = "Could not open the Markdown file.";
        return false;
    }

    const QByteArray markdown = file.readAll();
    QTextDocument document;
    document.setMarkdown(QString::fromUtf8(markdown));
    document.setDocumentMargin(0);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QPdfWriter writer(&buffer);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(18.0, 18.0, 18.0, 18.0), QPageLayout::Millimeter);
    writer.setResolution(300);
    writer.setTitle(QStringLiteral("Morpho document"));
    writer.setCreator(QStringLiteral("Morpho"));

    document.print(&writer);
    buffer.close();

    if (!buffer.data().startsWith("%PDF")) {
        error = "Could not render the Markdown file as PDF.";
        return false;
    }

    output.assign(buffer.data().begin(), buffer.data().end());
    return true;
}

#ifdef MORPHO_HAS_LIBREOFFICEKIT

std::mutex g_libreOfficeMutex;
LibreOfficeKit *g_libreOffice = nullptr;
std::string g_libreOfficeInitError;

std::string libreOfficeError(LibreOfficeKit *office) {
    if (office == nullptr || office->pClass->getError == nullptr) {
        return "LibreOfficeKit could not be initialized.";
    }

    char *message = office->pClass->getError(office);
    if (message == nullptr) {
        return "LibreOfficeKit reported an error.";
    }

    std::string result = message;
    office->pClass->freeError(message);
    return result;
}

LibreOfficeKit *libreOfficeInstance() {
    if (g_libreOffice != nullptr) {
        return g_libreOffice;
    }
    if (!g_libreOfficeInitError.empty()) {
        return nullptr;
    }

    qputenv("SAL_USE_VCLPLUGIN", "svp");
    const QString profilePath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
                                QStringLiteral("/libreoffice-profile");
    if (!QDir().mkpath(profilePath)) {
        g_libreOfficeInitError = "Could not create the LibreOffice profile directory.";
        return nullptr;
    }

    QByteArray profileUrl = fileUrl(std::filesystem::path(profilePath.toStdString())).toUtf8();
    if (!profileUrl.endsWith('/')) {
        profileUrl.append('/');
    }

    g_libreOffice = lok_init_2(MORPHO_LIBREOFFICE_PROGRAM_PATH, profileUrl.constData());
    if (g_libreOffice == nullptr) {
        g_libreOfficeInitError =
            "LibreOfficeKit could not be initialized. Is LibreOffice installed?";
        return nullptr;
    }

    return g_libreOffice;
}

bool convertOffice(const std::filesystem::path &input, const std::string &outputFormat,
                   std::vector<std::uint8_t> &output, std::string &error) {
    std::lock_guard<std::mutex> lock(g_libreOfficeMutex);

    LibreOfficeKit *office = libreOfficeInstance();
    if (office == nullptr) {
        error = g_libreOfficeInitError;
        return false;
    }

    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        error = "Could not create a temporary directory for LibreOffice.";
        return false;
    }

    const QString outputExtension = QString::fromStdString(extensionForOutput(outputFormat));
    const QString outputPath =
        temporaryDirectory.filePath(QStringLiteral("output") + outputExtension);
    const QByteArray inputUrl = fileUrl(input).toUtf8();
    const QByteArray outputUrl = fileUrl(std::filesystem::path(outputPath.toStdString())).toUtf8();

    LibreOfficeKitDocument *document = office->pClass->documentLoad(office, inputUrl.constData());
    if (document == nullptr) {
        error = libreOfficeError(office);
        return false;
    }

    const char *filter = nullptr;
    if (outputFormat == "pdf") {
        filter = "pdf";
    } else if (outputFormat == "docx") {
        filter = "docx";
    } else if (outputFormat == "odt") {
        filter = "odt";
    }

    const int saved = document->pClass->saveAs(document, outputUrl.constData(), filter, nullptr);
    document->pClass->destroy(document);

    if (saved == 0) {
        error = "LibreOfficeKit could not save the converted document.";
        return false;
    }

    QFile file(outputPath);
    if (!file.open(QIODevice::ReadOnly)) {
        error = "Could not read the LibreOffice output.";
        return false;
    }

    const QByteArray data = file.readAll();
    if (data.isEmpty()) {
        error = "LibreOffice produced an empty document.";
        return false;
    }

    output.assign(data.begin(), data.end());
    return true;
}

#endif

} // namespace

bool DocumentConverter::supportsInput(const std::filesystem::path &path) const {
    const std::string extension = lowerExtension(path);
    if (isMarkdown(extension)) {
        return true;
    }
#ifdef MORPHO_HAS_LIBREOFFICEKIT
    return isOfficeInput(extension);
#else
    return false;
#endif
}

bool DocumentConverter::usedLibreOffice() const {
#ifdef MORPHO_HAS_LIBREOFFICEKIT
    return g_libreOffice != nullptr;
#else
    return false;
#endif
}

ConversionResult DocumentConverter::convert(const ConversionRequest &request) {
    const std::string inputFormat = lowerExtension(request.inputPath);
    const std::string outputFormat = request.outputFormat;
    const std::string outputExtension = extensionForOutput(outputFormat);

    if (outputExtension.empty()) {
        return failure(request, "Unsupported document output format.");
    }

    std::error_code sizeError;
    std::uint64_t inputBytes = 0;
    const std::uintmax_t size = std::filesystem::file_size(request.inputPath, sizeError);
    if (!sizeError) {
        inputBytes = static_cast<std::uint64_t>(size);
    }

    std::vector<std::uint8_t> output;
    std::string error;

    if (isMarkdown(inputFormat)) {
        if (outputFormat != "pdf") {
            return failure(request, "Markdown can currently be converted to PDF only.");
        }
        if (!convertMarkdownToPdf(request.inputPath, output, error)) {
            return failure(request, std::move(error));
        }
    } else if (isOfficeInput(inputFormat)) {
#ifdef MORPHO_HAS_LIBREOFFICEKIT
        if (!convertOffice(request.inputPath, outputFormat, output, error)) {
            return failure(request, std::move(error));
        }
#else
        return failure(request, "LibreOfficeKit support is not available in this build.");
#endif
    } else {
        return failure(request, "Unsupported document input format.");
    }

    std::filesystem::path outputName = request.inputPath.filename();
    outputName.replace_extension(outputExtension);

    ConversionResult result;
    result.success = true;
    result.inputPath = request.inputPath;
    result.inputFormat = upperExtension(request.inputPath);
    result.outputFormat = outputFormat;
    result.outputFileName = outputName.string();
    result.inputBytes = inputBytes;
    result.data = std::move(output);
    return result;
}

} // namespace morpho::document
