// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#include "ui/app_icon.hpp"

#include <QSize>
#include <QString>
#include <array>

namespace morpho::ui {

QIcon appIcon() {
    QIcon icon;
    constexpr std::array<int, 7> kSizes = {16, 24, 32, 48, 64, 128, 256};

    for (const int size : kSizes) {
        icon.addFile(QStringLiteral(":/icons/morpho-%1.png").arg(size), QSize(size, size));
    }

    return icon;
}

} // namespace morpho::ui
