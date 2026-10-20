// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <QColor>
#include <QPalette>

namespace morpho::ui {

// Primary red sampled from the Morpho logo.
inline const QColor kBrandRed(0xD9, 0x22, 0x22);

inline QColor brandRedDark() {
    return kBrandRed.darker(115);
}

QPalette lightPalette();
void applyLightTheme();

} // namespace morpho::ui
