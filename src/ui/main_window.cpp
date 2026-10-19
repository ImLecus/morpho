// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "ui/main_window.hpp"

#include <QAction>
#include <QFont>
#include <QFrame>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

namespace morpho::ui {
namespace {

QLabel *makeLabel(const QString &text, QWidget *parent) {
    auto *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignCenter);
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle(tr("Morpho"));
    setMinimumSize(640, 420);
    resize(900, 560);

    auto *central = new QWidget(this);
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
    dropZone->setFrameShape(QFrame::StyledPanel);
    dropZone->setFrameShadow(QFrame::Sunken);
    dropZone->setMinimumHeight(240);

    auto *dropLayout = new QVBoxLayout(dropZone);
    dropLayout->setContentsMargins(24, 24, 24, 24);
    dropLayout->setSpacing(8);

    auto *dropTitle = makeLabel(tr("Drop files here"), dropZone);
    scaleFont(dropTitle, 1.35);

    auto *dropHint =
        makeLabel(tr("File selection and conversion arrive in the next milestone."), dropZone);

    dropLayout->addStretch();
    dropLayout->addWidget(dropTitle);
    dropLayout->addWidget(dropHint);
    dropLayout->addStretch();

    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addWidget(dropZone, 1);
    setCentralWidget(central);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    auto *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    statusBar()->showMessage(tr("Ready. Files stay on this device."));
}

} // namespace morpho::ui
