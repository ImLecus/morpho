// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "ui/main_window.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <utility>

#include <QAction>
#include <QByteArray>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMetaObject>
#include <QMimeData>
#include <QPushButton>
#include <QStatusBar>
#include <QString>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/app_icon.hpp"
#include "ui/theme.hpp"

namespace morpho::ui {
namespace {

bool isJpegPath(const QString &path) {
    const QString suffix = QFileInfo(path).suffix();
    return suffix.compare(QStringLiteral("jpg"), Qt::CaseInsensitive) == 0 ||
           suffix.compare(QStringLiteral("jpeg"), Qt::CaseInsensitive) == 0;
}

std::filesystem::path toPath(const QString &path) {
    return std::filesystem::path(QFile::encodeName(path).toStdString());
}

QString fromPath(const std::filesystem::path &path) {
    const std::string bytes = path.string();
    return QFile::decodeName(QByteArray(bytes.data(), static_cast<int>(bytes.size())));
}

QString uniqueOutputPath(const QString &inputPath) {
    const QFileInfo input(inputPath);
    const QDir directory = input.absoluteDir();
    QString base = input.completeBaseName();

    if (base.isEmpty()) {
        base = QStringLiteral("converted");
    }

    QString candidate = directory.filePath(base + QStringLiteral(".png"));
    int counter = 1;
    while (QFileInfo::exists(candidate)) {
        candidate = directory.filePath(QStringLiteral("%1 (%2).png").arg(base).arg(counter));
        ++counter;
    }

    return candidate;
}

bool hasLocalFileUrls(const QMimeData *mimeData) {
    if (mimeData == nullptr) {
        return false;
    }

    for (const QUrl &url : mimeData->urls()) {
        if (url.isLocalFile()) {
            return true;
        }
    }

    return false;
}

QStringList jpegFilesFromUrls(const QList<QUrl> &urls, int &skipped) {
    QStringList files;

    for (const QUrl &url : urls) {
        if (!url.isLocalFile()) {
            ++skipped;
            continue;
        }

        const QString path = url.toLocalFile();
        if (isJpegPath(path) && QFileInfo(path).isFile()) {
            files.append(path);
        } else {
            ++skipped;
        }
    }

    return files;
}

QLabel *makeLabel(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
    label->setTextFormat(Qt::PlainText);
    label->setWordWrap(true);
    return label;
}

void scaleFont(QLabel *label, double factor) {
    QFont font = label->font();
    if (font.pointSizeF() > 0.0) {
        font.setPointSizeF(font.pointSizeF() * factor);
    } else if (font.pixelSize() > 0) {
        font.setPixelSize(static_cast<int>(font.pixelSize() * factor));
    }
    label->setFont(font);
}

} // namespace

MainWindow::MainWindow(ConversionService &conversionService, QWidget *parent)
    : QMainWindow(parent), m_conversionService(conversionService) {
    setWindowTitle(tr("Morpho"));
    setWindowIcon(appIcon());
    setMinimumSize(720, 680);
    resize(960, 680);
    setAcceptDrops(true);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));
    central->setStyleSheet(QStringLiteral("QWidget#central { background-color: #FFFFFF; }"));
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(48, 48, 48, 48);
    layout->setSpacing(16);

    auto *title = makeLabel(tr("Convert anything. Locally."), central);
    scaleFont(title, 2.0);
    QFont titleFont = title->font();
    titleFont.setBold(true);
    title->setFont(titleFont);

    auto *subtitle = makeLabel(tr("Convert files offline. Nothing leaves this device."), central);

    auto *dropZone = new QFrame(central);
    dropZone->setObjectName(QStringLiteral("dropZone"));
    dropZone->setAccessibleName(tr("File drop area"));
    dropZone->setAttribute(Qt::WA_StyledBackground, true);
    dropZone->setFrameShape(QFrame::NoFrame);
    dropZone->setMinimumHeight(240);
    dropZone->setStyleSheet(QStringLiteral("QFrame#dropZone { background-color: #FFFFFF;"
                                           " border: 2px solid %1; border-radius: 12px; }")
                                .arg(kBrandRed.name()));

    auto *dropLayout = new QVBoxLayout(dropZone);
    dropLayout->setContentsMargins(24, 24, 24, 24);
    dropLayout->setSpacing(8);

    auto *dropTitle = makeLabel(tr("Drop JPG files here"), dropZone);
    scaleFont(dropTitle, 1.35);

    m_dropHint = makeLabel(
        tr("Drop JPG or JPEG files here. PNG files are written next to the originals."), dropZone);

    auto *chooseButton = new QPushButton(tr("Choose JPG files"), dropZone);
    chooseButton->setToolTip(tr("Select one or more JPG or JPEG files"));
    connect(chooseButton, &QPushButton::clicked, this, &MainWindow::chooseFiles);

    dropLayout->addStretch();
    dropLayout->addWidget(dropTitle);
    dropLayout->addWidget(m_dropHint);
    dropLayout->addWidget(chooseButton, 0, Qt::AlignHCenter);
    dropLayout->addStretch();

    auto *conversionPanel = new QFrame(central);
    conversionPanel->setObjectName(QStringLiteral("conversionPanel"));
    conversionPanel->setAccessibleName(tr("Conversion format"));
    conversionPanel->setAttribute(Qt::WA_StyledBackground, true);
    conversionPanel->setFrameShape(QFrame::NoFrame);
    conversionPanel->setMinimumHeight(140);
    conversionPanel->setStyleSheet(
        QStringLiteral(
            "QFrame#conversionPanel { background-color: %1; border-radius: 12px; }"
            "QFrame#conversionPanel QLabel { color: #FFFFFF; font-weight: 600; }"
            "QFrame#conversionPanel QComboBox { background-color: %2; color: #FFFFFF;"
            " border: 1px solid rgba(255, 255, 255, 0.28); border-radius: 6px;"
            " padding: 6px 10px; padding-right: 30px; min-height: 24px; }"
            "QFrame#conversionPanel QComboBox::drop-down { subcontrol-origin: padding;"
            " subcontrol-position: top right; width: 28px; border: none;"
            " background: transparent; }"
            "QFrame#conversionPanel QComboBox::down-arrow {"
            " image: url(:/icons/chevron-down.png); width: 12px; height: 12px; }"
            "QFrame#conversionPanel QComboBox QAbstractItemView { background-color: %2;"
            " color: #FFFFFF; selection-background-color: %3; selection-color: #FFFFFF; }")
            .arg(kBrandRed.name(), brandRedDark().name(), kBrandRed.darker(140).name()));

    auto *conversionLayout = new QHBoxLayout(conversionPanel);
    conversionLayout->setContentsMargins(32, 24, 32, 24);
    conversionLayout->setSpacing(12);

    auto *fromLabel = new QLabel(tr("From"), conversionPanel);
    auto *fromCombo = new QComboBox(conversionPanel);
    fromCombo->setObjectName(QStringLiteral("fromFormatCombo"));
    fromCombo->setAccessibleName(tr("Input format"));
    fromCombo->setMinimumWidth(160);
    fromCombo->setToolTip(tr("JPG and JPEG inputs are supported for now."));
    fromCombo->addItem(tr("JPG"), QStringLiteral("jpg"));
    fromCombo->addItem(tr("JPEG"), QStringLiteral("jpeg"));
    fromLabel->setBuddy(fromCombo);

    auto *toLabel = new QLabel(tr("To"), conversionPanel);
    auto *toCombo = new QComboBox(conversionPanel);
    toCombo->setObjectName(QStringLiteral("toFormatCombo"));
    toCombo->setAccessibleName(tr("Output format"));
    toCombo->setMinimumWidth(160);
    toCombo->setToolTip(tr("PNG output is supported for now."));
    toCombo->addItem(tr("PNG"), QStringLiteral("png"));
    toLabel->setBuddy(toCombo);

    auto *arrow = new QLabel(QStringLiteral("→"), conversionPanel);
    scaleFont(arrow, 1.5);
    QFont arrowFont = arrow->font();
    arrowFont.setBold(true);
    arrow->setFont(arrowFont);

    conversionLayout->addStretch();
    conversionLayout->addWidget(fromLabel);
    conversionLayout->addWidget(fromCombo);
    conversionLayout->addWidget(arrow);
    conversionLayout->addWidget(toLabel);
    conversionLayout->addWidget(toCombo);
    conversionLayout->addStretch();

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(dropZone, 1);
    layout->addWidget(conversionPanel);
    setCentralWidget(central);

    menuBar()->setStyleSheet(
        QStringLiteral("QMenuBar { background-color: #FFFFFF; color: #1F1F1F; }"
                       "QMenuBar::item:selected { background-color: #F2F2F2; }"));
    statusBar()->setStyleSheet(
        QStringLiteral("QStatusBar { background-color: #FFFFFF; color: #555555; }"));

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *openAction = fileMenu->addAction(tr("&Open JPG files..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::chooseFiles);
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    statusBar()->showMessage(tr("Ready. Drop JPG files to convert them to PNG."));

    for (QWidget *widget : findChildren<QWidget *>()) {
        widget->setAcceptDrops(true);
        widget->installEventFilter(this);
    }
}

MainWindow::~MainWindow() {
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
    if (hasLocalFileUrls(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event) {
    if (hasLocalFileUrls(event->mimeData())) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dropEvent(QDropEvent *event) {
    const QMimeData *mimeData = event->mimeData();
    if (mimeData == nullptr) {
        event->ignore();
        return;
    }

    int skipped = 0;
    const QStringList files = jpegFilesFromUrls(mimeData->urls(), skipped);
    if (files.isEmpty()) {
        m_dropHint->setText(tr("Only JPG and JPEG files are supported right now."));
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    if (skipped > 0) {
        statusBar()->showMessage(skipped == 1 ? tr("Skipped 1 unsupported file.")
                                              : tr("Skipped %1 unsupported files.").arg(skipped),
                                 5000);
    }
    startConversions(files);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    switch (event->type()) {
    case QEvent::DragEnter:
        dragEnterEvent(static_cast<QDragEnterEvent *>(event));
        return true;
    case QEvent::DragMove:
        dragMoveEvent(static_cast<QDragMoveEvent *>(event));
        return true;
    case QEvent::Drop:
        dropEvent(static_cast<QDropEvent *>(event));
        return true;
    default:
        break;
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::chooseFiles() {
    if (m_converting) {
        statusBar()->showMessage(tr("A conversion is already running."), 5000);
        return;
    }

    const QStringList files = QFileDialog::getOpenFileNames(this, tr("Choose JPG files"), QString(),
                                                            tr("JPEG images (*.jpg *.jpeg)"));
    if (!files.isEmpty()) {
        startConversions(files);
    }
}

void MainWindow::startConversions(const QStringList &files) {
    if (m_converting) {
        statusBar()->showMessage(tr("A conversion is already running."), 5000);
        return;
    }

    QStringList jpegFiles;
    int skipped = 0;
    for (const QString &file : files) {
        if (isJpegPath(file) && QFileInfo(file).isFile()) {
            jpegFiles.append(file);
        } else {
            ++skipped;
        }
    }

    if (jpegFiles.isEmpty()) {
        m_dropHint->setText(tr("Only JPG and JPEG files are supported right now."));
        return;
    }

    if (skipped > 0) {
        statusBar()->showMessage(skipped == 1 ? tr("Skipped 1 unsupported file.")
                                              : tr("Skipped %1 unsupported files.").arg(skipped),
                                 5000);
    }

    if (m_worker.joinable()) {
        m_worker.join();
    }

    const int count = static_cast<int>(jpegFiles.size());
    m_converting = true;
    m_dropHint->setText(count == 1 ? tr("Converting 1 file to PNG...")
                                   : tr("Converting %1 files to PNG...").arg(count));
    statusBar()->showMessage(tr("Converting..."));

    auto *service = &m_conversionService;
    m_worker = std::jthread([this, service, jpegFiles] {
        QVector<ConversionResult> results;

        try {
            results.reserve(jpegFiles.size());

            for (const QString &file : jpegFiles) {
                ConversionRequest request;
                ConversionResult result;

                try {
                    request.inputPath = toPath(file);
                    request.outputPath = toPath(uniqueOutputPath(file));
                    result = service->convert(request);
                } catch (const std::exception &exception) {
                    result.inputPath = request.inputPath;
                    result.outputPath = request.outputPath;
                    result.error = exception.what();
                } catch (...) {
                    result.inputPath = request.inputPath;
                    result.outputPath = request.outputPath;
                    result.error = "Unknown conversion error.";
                }

                results.append(result);
            }
        } catch (const std::exception &exception) {
            ConversionResult failure;
            failure.error = exception.what();
            results.append(failure);
        } catch (...) {
            ConversionResult failure;
            failure.error = "Unknown conversion error.";
            results.append(failure);
        }

        QMetaObject::invokeMethod(
            this,
            [this, results = std::move(results)]() mutable {
                conversionFinished(std::move(results));
            },
            Qt::QueuedConnection);
    });
}

void MainWindow::conversionFinished(QVector<ConversionResult> results) {
    m_converting = false;

    if (results.isEmpty()) {
        m_dropHint->setText(tr("No files were converted."));
        return;
    }

    int converted = 0;
    QStringList errors;
    for (const ConversionResult &result : results) {
        if (result.success) {
            ++converted;
            continue;
        }

        const QString inputName = QFileInfo(fromPath(result.inputPath)).fileName();
        const QString detail = QString::fromStdString(result.error);
        errors.append(tr("%1: %2").arg(inputName, detail.isEmpty() ? tr("Unknown error") : detail));
    }

    const int total = static_cast<int>(results.size());
    if (converted == total) {
        m_dropHint->setText(
            converted == 1
                ? tr("Converted 1 file to PNG. The PNG file is next to the original.")
                : tr("Converted %1 files to PNG. The PNG files are next to the originals.")
                      .arg(converted));
        statusBar()->showMessage(tr("Conversion complete."), 8000);
    } else if (converted == 0) {
        m_dropHint->setText(
            tr("Could not convert the dropped files. %1").arg(errors.join(QStringLiteral("; "))));
        statusBar()->showMessage(tr("Conversion failed."), 8000);
    } else {
        m_dropHint->setText(tr("Converted %1 of %2 files. %3")
                                .arg(converted)
                                .arg(total)
                                .arg(errors.join(QStringLiteral("; "))));
        statusBar()->showMessage(tr("Conversion finished with errors."), 8000);
    }
}

} // namespace morpho::ui
