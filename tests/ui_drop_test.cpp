// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/image/jpeg_to_png.hpp"
#include "ui/main_window.hpp"
#include "ui/theme.hpp"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QMimeData>
#include <QPalette>
#include <QPointF>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>
#include <QWidget>

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: morpho_ui_drop_test <input.jpg>\n";
        return 2;
    }

    QApplication application(argc, argv);
    morpho::ui::applyLightTheme();

    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        std::cerr << "Could not create a temporary directory.\n";
        return 1;
    }

    const QString fixture = QString::fromLocal8Bit(argv[1]);
    const QString input = temporaryDirectory.filePath(QStringLiteral("tiny.jpg"));
    if (!QFile::copy(fixture, input)) {
        std::cerr << "Could not copy the JPEG fixture.\n";
        return 1;
    }

    morpho::image::JpegToPngConverter converter;
    morpho::ui::MainWindow window(converter);
    window.show();

    QWidget *dropTarget = window.findChild<QWidget *>(QStringLiteral("dropZone"));
    auto *conversionPanel = window.findChild<QFrame *>(QStringLiteral("conversionPanel"));
    auto *fromCombo = window.findChild<QComboBox *>(QStringLiteral("fromFormatCombo"));
    auto *toCombo = window.findChild<QComboBox *>(QStringLiteral("toFormatCombo"));

    if (dropTarget == nullptr || conversionPanel == nullptr || fromCombo == nullptr ||
        toCombo == nullptr) {
        std::cerr << "Could not find the drop area or conversion controls.\n";
        return 1;
    }

    if (fromCombo->currentData().toString() != QStringLiteral("jpg") ||
        toCombo->currentData().toString() != QStringLiteral("png")) {
        std::cerr << "The conversion controls have unexpected defaults.\n";
        return 1;
    }

    if (!conversionPanel->styleSheet().contains(QStringLiteral("#D92222"), Qt::CaseInsensitive)) {
        std::cerr << "The conversion panel does not use the logo color.\n";
        return 1;
    }

    if (!conversionPanel->styleSheet().contains(morpho::ui::brandRedDark().name(),
                                                Qt::CaseInsensitive) ||
        !conversionPanel->styleSheet().contains(QStringLiteral("color: #FFFFFF"),
                                                Qt::CaseInsensitive)) {
        std::cerr << "The format selects do not use the dark brand color and white text.\n";
        return 1;
    }

    if (!dropTarget->styleSheet().contains(QStringLiteral("border: 2px solid"),
                                           Qt::CaseInsensitive) ||
        !dropTarget->styleSheet().contains(QStringLiteral("#D92222"), Qt::CaseInsensitive)) {
        std::cerr << "The drop area does not have the brand border.\n";
        return 1;
    }

    if (QApplication::palette().color(QPalette::Window) != QColor(Qt::white)) {
        std::cerr << "The application background is not white.\n";
        return 1;
    }

    QMimeData mimeData;
    mimeData.setUrls({QUrl::fromLocalFile(input)});

    QDragEnterEvent enterEvent(QPoint(10, 10), Qt::CopyAction, &mimeData, Qt::LeftButton,
                               Qt::NoModifier);
    QCoreApplication::sendEvent(dropTarget, &enterEvent);
    if (!enterEvent.isAccepted()) {
        std::cerr << "The drag-enter event was not accepted.\n";
        return 1;
    }

    QDropEvent dropEvent(QPointF(10.0, 10.0), Qt::CopyAction, &mimeData, Qt::LeftButton,
                         Qt::NoModifier);
    QCoreApplication::sendEvent(dropTarget, &dropEvent);

    if (!dropEvent.isAccepted()) {
        std::cerr << "The drop event was not accepted.\n";
        return 1;
    }

    const QString output = temporaryDirectory.filePath(QStringLiteral("tiny.png"));
    QElapsedTimer timer;
    timer.start();

    while (!QFileInfo::exists(output) && timer.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }

    if (!QFileInfo::exists(output)) {
        std::cerr << "The drag-and-drop conversion did not produce a PNG.\n";
        return 1;
    }

    QFile file(output);
    if (!file.open(QIODevice::ReadOnly)) {
        std::cerr << "Could not open the generated PNG.\n";
        return 1;
    }

    const QByteArray signature = file.read(8);
    if (signature != QByteArray::fromHex("89504e470d0a1a0a")) {
        std::cerr << "The generated file is not a PNG.\n";
        return 1;
    }

    std::cout << "Drag-and-drop conversion produced a PNG.\n";
    return 0;
}
