// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "ui/theme.hpp"

#include <QApplication>
#include <QString>

namespace morpho::ui {

QPalette lightPalette() {
    QPalette palette;
    palette.setColor(QPalette::Window, Qt::white);
    palette.setColor(QPalette::WindowText, QColor(0x1F, 0x1F, 0x1F));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor(0xF7, 0xF7, 0xF7));
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, QColor(0x1F, 0x1F, 0x1F));
    palette.setColor(QPalette::Text, QColor(0x1F, 0x1F, 0x1F));
    palette.setColor(QPalette::Button, QColor(0xF5, 0xF5, 0xF5));
    palette.setColor(QPalette::ButtonText, QColor(0x1F, 0x1F, 0x1F));
    palette.setColor(QPalette::BrightText, Qt::white);
    palette.setColor(QPalette::Link, kBrandRed.darker(110));
    palette.setColor(QPalette::Highlight, kBrandRed);
    palette.setColor(QPalette::HighlightedText, Qt::white);
    return palette;
}

void applyLightTheme() {
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setPalette(lightPalette());
}

} // namespace morpho::ui
