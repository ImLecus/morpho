// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string_view>

#include <QApplication>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QString>
#include <QTimer>
#include <QWindow>

#include "backends/morpho_converter.hpp"
#include "ui/app_icon.hpp"
#include "ui/main_window.hpp"
#include "ui/theme.hpp"

namespace {

constexpr std::string_view kVersion = MORPHO_VERSION;

void printUsage(std::ostream &output) {
    output << "Usage: morpho [--version] [--help] [--smoke-test]\n";
}

} // namespace

int main(int argc, char *argv[]) {
    bool smokeTest = false;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index];

        if (argument == "--version" || argument == "-v") {
            std::cout << "Morpho " << kVersion << '\n';
            return 0;
        }

        if (argument == "--help" || argument == "-h") {
            printUsage(std::cout);
            return 0;
        }

        if (argument == "--smoke-test") {
            smokeTest = true;
        }
    }

    QApplication application(argc, argv);
    morpho::ui::applyLightTheme();
    QCoreApplication::setOrganizationName(QStringLiteral("Morpho"));
    QCoreApplication::setApplicationName(QStringLiteral("Morpho"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(MORPHO_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("morpho"));
    const QIcon resourceIcon = morpho::ui::appIcon();
    QApplication::setWindowIcon(resourceIcon);

    morpho::MorphoConverter converter;
    int exitCode = 0;
    {
        morpho::ui::MainWindow window(converter);
        window.show();
        if (auto *handle = window.windowHandle()) {
            handle->setIcon(resourceIcon);
        }

        if (smokeTest) {
            if (QGuiApplication::windowIcon().isNull()) {
                std::cerr << "Morpho application icon failed to load\n";
                return 1;
            }

            QTimer::singleShot(0, &application, &QCoreApplication::quit);
        }

        exitCode = application.exec();
    }

    // Check after the window is destroyed: its destructor joins the worker.
    const bool hardExit = converter.usedLibreOffice();
    if (hardExit) {
        // LibreOfficeKit 24.2 can crash in its shutdown path after the office
        // instance has been used. Qt objects above are already destroyed; let
        // the OS reclaim the remaining process state instead of running it.
        std::fflush(nullptr);
        std::_Exit(exitCode);
    }

    return exitCode;
}
