// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "ui/main_window.hpp"

#include <iostream>
#include <string_view>

#include <QApplication>
#include <QCoreApplication>
#include <QString>
#include <QTimer>

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
    QCoreApplication::setOrganizationName(QStringLiteral("Morpho"));
    QCoreApplication::setApplicationName(QStringLiteral("Morpho"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(MORPHO_VERSION));

    morpho::ui::MainWindow window;
    window.show();

    if (smokeTest) {
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
