// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <QMainWindow>

namespace morpho::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
};

} // namespace morpho::ui
