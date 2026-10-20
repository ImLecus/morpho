// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "ui/main_window.hpp"

#include <exception>
#include <filesystem>
#include <memory>
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
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMetaObject>
#include <QMimeData>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollArea>
#include <QSize>
#include <QSizePolicy>
#include <QStandardPaths>
#include <QStatusBar>
#include <QString>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include "ui/app_icon.hpp"
#include "ui/theme.hpp"

namespace morpho::ui {
namespace {

std::filesystem::path toPath(const QString &path) {
    return std::filesystem::path(QFile::encodeName(path).toStdString());
}

QString fromPath(const std::filesystem::path &path) {
    const std::string bytes = path.string();
    return QFile::decodeName(QByteArray(bytes.data(), static_cast<int>(bytes.size())));
}

QString cleanNumber(double value, int decimals) {
    QString text = QString::number(value, 'f', decimals);
    if (text.contains(QLatin1Char('.'))) {
        while (text.endsWith(QLatin1Char('0'))) {
            text.chop(1);
        }
        if (text.endsWith(QLatin1Char('.'))) {
            text.chop(1);
        }
    }
    return text;
}

QString formatBytes(std::uint64_t bytes) {
    constexpr double kKibibyte = 1024.0;
    constexpr double kMebibyte = kKibibyte * 1024.0;
    constexpr double kGibibyte = kMebibyte * 1024.0;

    if (bytes < static_cast<std::uint64_t>(kKibibyte)) {
        return QStringLiteral("%1b").arg(static_cast<qulonglong>(bytes));
    }
    if (bytes < static_cast<std::uint64_t>(kMebibyte)) {
        const double value = static_cast<double>(bytes) / kKibibyte;
        return QStringLiteral("%1kb").arg(cleanNumber(value, value < 10.0 ? 1 : 0));
    }
    if (bytes < static_cast<std::uint64_t>(kGibibyte)) {
        const double value = static_cast<double>(bytes) / kMebibyte;
        return QStringLiteral("%1mb").arg(cleanNumber(value, value < 10.0 ? 1 : 0));
    }

    const double value = static_cast<double>(bytes) / kGibibyte;
    return QStringLiteral("%1gb").arg(cleanNumber(value, value < 10.0 ? 1 : 0));
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
    setMinimumSize(1040, 680);
    resize(1280, 720);
    setAcceptDrops(true);

    auto *central = new QWidget(this);
    central->setObjectName(QStringLiteral("central"));
    central->setStyleSheet(QStringLiteral("QWidget#central { background-color: #FFFFFF; }"));

    auto *rootLayout = new QHBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *mainArea = new QWidget(central);
    mainArea->setObjectName(QStringLiteral("mainArea"));
    mainArea->setStyleSheet(QStringLiteral("QWidget#mainArea { background-color: #FFFFFF; }"));

    auto *layout = new QVBoxLayout(mainArea);
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

    auto *dropTitle = makeLabel(tr("Drop images here"), dropZone);
    scaleFont(dropTitle, 1.35);

    m_dropHint = makeLabel(tr("Drop PNG, JPEG, WEBP, AVIF, TIFF, BMP, or GIF files here. Converted "
                              "images appear in the sidebar."),
                           dropZone);

    auto *chooseButton = new QPushButton(tr("Choose images"), dropZone);
    chooseButton->setToolTip(tr("Select one or more image files"));
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

    auto *conversionLayout = new QVBoxLayout(conversionPanel);
    conversionLayout->setContentsMargins(32, 20, 32, 20);
    conversionLayout->setSpacing(6);

    auto *controlsLayout = new QHBoxLayout();
    controlsLayout->setSpacing(12);

    auto *toLabel = new QLabel(tr("Convert to"), conversionPanel);
    m_outputFormat = new QComboBox(conversionPanel);
    m_outputFormat->setObjectName(QStringLiteral("toFormatCombo"));
    m_outputFormat->setAccessibleName(tr("Output format"));
    m_outputFormat->setMinimumWidth(200);
    m_outputFormat->setToolTip(tr("Choose the output image format."));
    m_outputFormat->addItem(tr("PNG"), QStringLiteral("png"));
    m_outputFormat->addItem(tr("JPEG"), QStringLiteral("jpeg"));
    m_outputFormat->addItem(tr("WEBP"), QStringLiteral("webp"));
    m_outputFormat->addItem(tr("AVIF"), QStringLiteral("avif"));
    m_outputFormat->addItem(tr("TIFF"), QStringLiteral("tiff"));
    m_outputFormat->addItem(tr("BMP"), QStringLiteral("bmp"));
    m_outputFormat->addItem(tr("GIF"), QStringLiteral("gif"));
    toLabel->setBuddy(m_outputFormat);

    controlsLayout->addStretch();
    controlsLayout->addWidget(toLabel);
    controlsLayout->addWidget(m_outputFormat);
    controlsLayout->addStretch();
    conversionLayout->addLayout(controlsLayout);

    auto *inputHint = new QLabel(tr("Input format is detected automatically."), conversionPanel);
    inputHint->setAlignment(Qt::AlignCenter);
    inputHint->setStyleSheet(QStringLiteral("color: rgba(255, 255, 255, 0.75);"
                                            " font-weight: 400;"));
    conversionLayout->addWidget(inputHint);

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(dropZone, 1);
    layout->addWidget(conversionPanel);

    auto *sidebar = new QFrame(central);
    sidebar->setObjectName(QStringLiteral("convertedSidebar"));
    sidebar->setAccessibleName(tr("Converted files"));
    sidebar->setAttribute(Qt::WA_StyledBackground, true);
    sidebar->setFixedWidth(340);
    sidebar->setStyleSheet(QStringLiteral(
        "QFrame#convertedSidebar { background-color: #FFFFFF; border-left: 1px solid #E5E5E5; }"));

    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(24, 32, 24, 24);
    sidebarLayout->setSpacing(8);

    auto *sidebarTitle = new QLabel(tr("Converted files"), sidebar);
    scaleFont(sidebarTitle, 1.35);
    QFont sidebarTitleFont = sidebarTitle->font();
    sidebarTitleFont.setBold(true);
    sidebarTitle->setFont(sidebarTitleFont);

    auto *sidebarHint = new QLabel(tr("Files stay in memory until you download them."), sidebar);
    sidebarHint->setTextFormat(Qt::PlainText);
    sidebarHint->setWordWrap(true);
    sidebarHint->setStyleSheet(QStringLiteral("color: #666666;"));

    m_convertedScroll = new QScrollArea(sidebar);
    m_convertedScroll->setObjectName(QStringLiteral("convertedScroll"));
    m_convertedScroll->setWidgetResizable(true);
    m_convertedScroll->setFrameShape(QFrame::NoFrame);
    m_convertedScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_convertedScroll->setStyleSheet(
        QStringLiteral("QScrollArea { background-color: #FFFFFF; border: none; }"));

    m_convertedContainer = new QWidget(m_convertedScroll);
    m_convertedContainer->setObjectName(QStringLiteral("convertedContainer"));
    m_convertedContainer->setStyleSheet(
        QStringLiteral("QWidget#convertedContainer { background-color: #FFFFFF; }"));

    m_convertedLayout = new QVBoxLayout(m_convertedContainer);
    m_convertedLayout->setContentsMargins(0, 0, 0, 0);
    m_convertedLayout->setSpacing(8);
    m_convertedLayout->addStretch();
    m_convertedScroll->setWidget(m_convertedContainer);

    sidebarLayout->addWidget(sidebarTitle);
    sidebarLayout->addWidget(sidebarHint);
    sidebarLayout->addSpacing(8);
    sidebarLayout->addWidget(m_convertedScroll, 1);

    rootLayout->addWidget(mainArea, 1);
    rootLayout->addWidget(sidebar);
    setCentralWidget(central);

    menuBar()->setStyleSheet(
        QStringLiteral("QMenuBar { background-color: #FFFFFF; color: #1F1F1F; }"
                       "QMenuBar::item:selected { background-color: #F2F2F2; }"));
    statusBar()->setStyleSheet(
        QStringLiteral("QStatusBar { background-color: #FFFFFF; color: #555555; }"));

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *openAction = fileMenu->addAction(tr("&Open images..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::chooseFiles);
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    statusBar()->showMessage(tr("Ready. Drop images to convert them."));

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

QStringList MainWindow::supportedFiles(const QMimeData *mimeData, int &skipped) const {
    QStringList files;
    if (mimeData == nullptr) {
        return files;
    }

    for (const QUrl &url : mimeData->urls()) {
        if (!url.isLocalFile()) {
            ++skipped;
            continue;
        }

        const QString path = url.toLocalFile();
        if (QFileInfo(path).isFile() && m_conversionService.supportsInput(toPath(path))) {
            files.append(path);
        } else {
            ++skipped;
        }
    }

    return files;
}

void MainWindow::dropEvent(QDropEvent *event) {
    int skipped = 0;
    const QStringList files = supportedFiles(event->mimeData(), skipped);
    if (files.isEmpty()) {
        m_dropHint->setText(
            tr("Unsupported image format. Use PNG, JPEG, WEBP, AVIF, TIFF, BMP, or GIF."));
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

    const QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Choose images"), QString(),
        tr("Images (*.png *.jpg *.jpeg *.jpe *.jfif *.webp *.avif *.tif *.tiff *.bmp *.gif);;"
           "All files (*)"));
    if (!files.isEmpty()) {
        startConversions(files);
    }
}

void MainWindow::startConversions(const QStringList &files) {
    if (m_converting) {
        statusBar()->showMessage(tr("A conversion is already running."), 5000);
        return;
    }

    QStringList imageFiles;
    int skipped = 0;
    for (const QString &file : files) {
        if (QFileInfo(file).isFile() && m_conversionService.supportsInput(toPath(file))) {
            imageFiles.append(file);
        } else {
            ++skipped;
        }
    }

    if (imageFiles.isEmpty()) {
        m_dropHint->setText(
            tr("Unsupported image format. Use PNG, JPEG, WEBP, AVIF, TIFF, BMP, or GIF."));
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

    const QString outputFormat = m_outputFormat->currentData().toString();
    if (outputFormat.isEmpty()) {
        m_dropHint->setText(tr("Choose an output format first."));
        return;
    }

    const int count = static_cast<int>(imageFiles.size());
    m_converting = true;
    m_dropHint->setText(count == 1 ? tr("Converting 1 image...")
                                   : tr("Converting %1 images...").arg(count));
    statusBar()->showMessage(tr("Converting..."));

    const std::string targetFormat = outputFormat.toStdString();
    auto *service = &m_conversionService;
    m_worker = std::jthread([this, service, imageFiles, targetFormat] {
        QVector<ConversionResult> results;

        try {
            results.reserve(imageFiles.size());

            for (const QString &file : imageFiles) {
                ConversionRequest request;
                ConversionResult result;

                try {
                    request.inputPath = toPath(file);
                    request.outputFormat = targetFormat;
                    result = service->convert(request);
                } catch (const std::exception &exception) {
                    result.inputPath = request.inputPath;
                    result.error = exception.what();
                } catch (...) {
                    result.inputPath = request.inputPath;
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
    for (ConversionResult &result : results) {
        if (result.success) {
            ++converted;
            addConvertedFile(std::move(result));
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
                ? tr("Converted 1 image. It is ready in the sidebar.")
                : tr("Converted %1 images. They are ready in the sidebar.").arg(converted));
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

void MainWindow::addConvertedFile(ConversionResult result) {
    const QFileInfo inputInfo(fromPath(result.inputPath));
    QString baseName = inputInfo.completeBaseName();
    if (baseName.isEmpty()) {
        baseName = inputInfo.fileName();
    }

    const QString outputName = QString::fromStdString(result.outputFileName);
    const QString sourceFormat = QString::fromStdString(result.inputFormat).toUpper();
    const QString targetFormat = QString::fromStdString(result.outputFormat).toUpper();
    const qint64 byteCount = static_cast<qint64>(result.data.size());
    const QString oldSize =
        result.inputBytes == 0 ? QStringLiteral("?") : formatBytes(result.inputBytes);
    auto payload = std::make_shared<QByteArray>(reinterpret_cast<const char *>(result.data.data()),
                                                static_cast<int>(result.data.size()));

    auto *row = new QWidget(m_convertedContainer);
    row->setObjectName(QStringLiteral("convertedRow"));
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setMinimumHeight(76);
    row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    row->setStyleSheet(QStringLiteral(
        "QWidget#convertedRow { background-color: #FFFFFF; border: 1px solid #E5E5E5;"
        " border-radius: 8px; }"
        "QWidget#convertedRow QLabel#convertedName { color: #1F1F1F; font-weight: 600; }"
        "QWidget#convertedRow QLabel#convertedSize { color: #666666; }"));

    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(12, 10, 12, 10);
    rowLayout->setSpacing(8);

    auto *textLayout = new QVBoxLayout();
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto *nameLabel =
        new QLabel(QStringLiteral("%1 (%2 → %3)").arg(baseName, sourceFormat, targetFormat), row);
    nameLabel->setObjectName(QStringLiteral("convertedName"));
    nameLabel->setTextFormat(Qt::PlainText);
    nameLabel->setWordWrap(true);
    nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    auto *sizeLabel = new QLabel(
        QStringLiteral("%1 → %2").arg(oldSize, formatBytes(static_cast<std::uint64_t>(byteCount))),
        row);
    sizeLabel->setObjectName(QStringLiteral("convertedSize"));
    sizeLabel->setTextFormat(Qt::PlainText);

    textLayout->addWidget(nameLabel);
    textLayout->addWidget(sizeLabel);

    auto *downloadButton = new QPushButton(row);
    downloadButton->setObjectName(QStringLiteral("downloadButton"));
    downloadButton->setCursor(Qt::PointingHandCursor);
    downloadButton->setIcon(QIcon(QStringLiteral(":/icons/download.png")));
    downloadButton->setIconSize(QSize(24, 24));
    downloadButton->setFixedSize(40, 40);
    downloadButton->setToolTip(tr("Download"));
    downloadButton->setAccessibleName(tr("Download"));
    downloadButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    downloadButton->setStyleSheet(
        QStringLiteral("QPushButton { background-color: %1; border: none; border-radius: 8px;"
                       " padding: 0; }"
                       "QPushButton:hover { background-color: %2; }"
                       "QPushButton:pressed { background-color: %3; }"
                       "QPushButton:disabled { background-color: #E5E5E5; }")
            .arg(kBrandRed.name(), brandRedDark().name(), kBrandRed.darker(140).name()));

    rowLayout->addLayout(textLayout, 1);
    rowLayout->addWidget(downloadButton, 0, Qt::AlignRight | Qt::AlignVCenter);

    m_convertedLayout->insertWidget(m_convertedLayout->count() - 1, row);
    QTimer::singleShot(0, row, [this, row] { m_convertedScroll->ensureWidgetVisible(row); });

    row->setAcceptDrops(true);
    row->installEventFilter(this);
    for (QWidget *child : row->findChildren<QWidget *>()) {
        child->setAcceptDrops(true);
        child->installEventFilter(this);
    }

    connect(downloadButton, &QPushButton::clicked, this,
            [this, payload, outputName, downloadButton] {
                if (saveConvertedFile(*payload, outputName)) {
                    payload->clear();
                    payload->squeeze();
                    downloadButton->setEnabled(false);
                    downloadButton->setToolTip(tr("Downloaded"));
                    downloadButton->setAccessibleName(tr("Downloaded"));
                }
            });
}

bool MainWindow::saveConvertedFile(const QByteArray &bytes, const QString &fileName) {
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    const QString directory = downloads.isEmpty() ? QDir::homePath() : downloads;
    QString path =
        QFileDialog::getSaveFileName(this, tr("Save converted file"),
                                     QDir(directory).filePath(fileName), tr("PNG image (*.png)"));

    if (path.isEmpty()) {
        return false;
    }

    if (!path.endsWith(QStringLiteral(".png"), Qt::CaseInsensitive)) {
        path += QStringLiteral(".png");
    }

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(
            this, tr("Could not save file"),
            tr("Could not open %1: %2").arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    if (file.write(bytes) != bytes.size()) {
        file.cancelWriting();
        QMessageBox::warning(
            this, tr("Could not save file"),
            tr("Could not write %1: %2").arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    if (!file.commit()) {
        QMessageBox::warning(this, tr("Could not save file"),
                             tr("Could not finish writing %1: %2")
                                 .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }

    statusBar()->showMessage(tr("Saved %1").arg(QFileInfo(path).fileName()), 8000);
    return true;
}

} // namespace morpho::ui
