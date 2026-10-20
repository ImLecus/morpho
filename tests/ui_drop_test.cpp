// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "backends/morpho_converter.hpp"
#include "ui/main_window.hpp"
#include "ui/theme.hpp"

#include <QApplication>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QMimeData>
#include <QPalette>
#include <QPointF>
#include <QPushButton>
#include <QScrollArea>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>
#include <QWidget>

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: morpho_ui_drop_test <input.jpg> [input.mp4]\n";
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

    morpho::MorphoConverter converter;
    morpho::ui::MainWindow window(converter);
    window.show();

    QWidget *dropTarget = window.findChild<QWidget *>(QStringLiteral("dropZone"));
    auto *conversionPanel = window.findChild<QFrame *>(QStringLiteral("conversionPanel"));
    auto *toCombo = window.findChild<QComboBox *>(QStringLiteral("toFormatCombo"));
    auto *convertedScroll = window.findChild<QScrollArea *>(QStringLiteral("convertedScroll"));

    if (dropTarget == nullptr || conversionPanel == nullptr || toCombo == nullptr ||
        convertedScroll == nullptr) {
        std::cerr << "Could not find the drop area, output format, or sidebar.\n";
        return 1;
    }

    if (toCombo->count() != 19 || toCombo->currentData().toString() != QStringLiteral("png") ||
        toCombo->itemData(8).toString() != QStringLiteral("mp3") ||
        toCombo->itemData(15).toString() != QStringLiteral("mp4") ||
        toCombo->itemData(18).toString() != QStringLiteral("mov")) {
        std::cerr << "The output format list is incomplete or has an unexpected default.\n";
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

    QElapsedTimer timer;
    timer.start();

    while (window.findChildren<QWidget *>(QStringLiteral("convertedRow")).isEmpty() &&
           timer.elapsed() < 5000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        QThread::msleep(10);
    }

    const QList<QWidget *> rows = window.findChildren<QWidget *>(QStringLiteral("convertedRow"));
    if (rows.size() != 1) {
        std::cerr << "The drag-and-drop conversion did not appear in the sidebar.\n";
        return 1;
    }

    if (QFileInfo::exists(temporaryDirectory.filePath(QStringLiteral("tiny.png")))) {
        std::cerr << "The conversion wrote output to disk instead of keeping it in memory.\n";
        return 1;
    }

    QWidget *row = rows.first();
    QCoreApplication::processEvents();

    auto *nameLabel = row->findChild<QLabel *>(QStringLiteral("convertedName"));
    auto *sizeLabel = row->findChild<QLabel *>(QStringLiteral("convertedSize"));
    auto *downloadButton = row->findChild<QPushButton *>(QStringLiteral("downloadButton"));

    if (nameLabel == nullptr || sizeLabel == nullptr || downloadButton == nullptr) {
        std::cerr << "The sidebar row is missing its labels or download button.\n";
        return 1;
    }

    if (nameLabel->text() != QStringLiteral("tiny (JPEG → PNG)")) {
        std::cerr << "The sidebar row has an unexpected file name.\n";
        return 1;
    }

    if (sizeLabel->text().isEmpty() || !sizeLabel->text().contains(QStringLiteral("→")) ||
        !downloadButton->isEnabled() || !downloadButton->text().isEmpty() ||
        downloadButton->icon().isNull() || downloadButton->icon().pixmap(32, 32).isNull() ||
        downloadButton->toolTip() != QStringLiteral("Download")) {
        std::cerr << "The sidebar row has an unexpected size or download state.\n";
        return 1;
    }

    if (!downloadButton->isVisible() || downloadButton->width() < 40 ||
        downloadButton->x() + downloadButton->width() > row->width()) {
        std::cerr << "The download button is not visible inside the sidebar row.\n";
        return 1;
    }

    if (!downloadButton->styleSheet().contains(QStringLiteral("#D92222"), Qt::CaseInsensitive)) {
        std::cerr << "The download button does not use the primary color.\n";
        return 1;
    }

    if (argc == 3) {
        const QString mediaFixture = QString::fromLocal8Bit(argv[2]);
        const QString mediaInput = temporaryDirectory.filePath(QStringLiteral("tiny.mp4"));
        if (!QFile::copy(mediaFixture, mediaInput)) {
            std::cerr << "Could not copy the media fixture.\n";
            return 1;
        }

        int mp3Index = -1;
        for (int i = 0; i < toCombo->count(); ++i) {
            if (toCombo->itemData(i).toString() == QStringLiteral("mp3")) {
                mp3Index = i;
                break;
            }
        }
        if (mp3Index < 0) {
            std::cerr << "Could not find the MP3 output option.\n";
            return 1;
        }
        toCombo->setCurrentIndex(mp3Index);

        QMimeData mediaMimeData;
        mediaMimeData.setUrls({QUrl::fromLocalFile(mediaInput)});
        QDragEnterEvent mediaEnterEvent(QPoint(10, 10), Qt::CopyAction, &mediaMimeData,
                                        Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(dropTarget, &mediaEnterEvent);
        if (!mediaEnterEvent.isAccepted()) {
            std::cerr << "The media drag-enter event was not accepted.\n";
            return 1;
        }

        QDropEvent mediaDropEvent(QPointF(10.0, 10.0), Qt::CopyAction, &mediaMimeData,
                                  Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(dropTarget, &mediaDropEvent);
        if (!mediaDropEvent.isAccepted()) {
            std::cerr << "The media drop event was not accepted.\n";
            return 1;
        }

        QElapsedTimer mediaTimer;
        mediaTimer.start();
        while (window.findChildren<QWidget *>(QStringLiteral("convertedRow")).size() < 2 &&
               mediaTimer.elapsed() < 5000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
            QThread::msleep(10);
        }

        const QList<QWidget *> mediaRows =
            window.findChildren<QWidget *>(QStringLiteral("convertedRow"));
        if (mediaRows.size() != 2) {
            std::cerr << "The media conversion did not appear in the sidebar.\n";
            return 1;
        }

        if (QFileInfo::exists(temporaryDirectory.filePath(QStringLiteral("tiny.mp3")))) {
            std::cerr << "The media conversion wrote output to disk.\n";
            return 1;
        }

        bool foundMediaRow = false;
        for (QWidget *mediaRow : mediaRows) {
            auto *mediaName = mediaRow->findChild<QLabel *>(QStringLiteral("convertedName"));
            if (mediaName != nullptr && mediaName->text() == QStringLiteral("tiny (MP4 → MP3)")) {
                foundMediaRow = true;
                break;
            }
        }
        if (!foundMediaRow) {
            std::cerr << "The media sidebar row has an unexpected name.\n";
            return 1;
        }
    }

    std::cout << "Drag-and-drop conversions produced in-memory results in the sidebar.\n";
    return 0;
}
