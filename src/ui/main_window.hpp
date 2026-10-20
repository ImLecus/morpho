// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <morpho/core/conversion.hpp>

#include <QMainWindow>
#include <QStringList>
#include <QVector>

#include <thread>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QLabel;

namespace morpho::ui {

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(ConversionService &conversionService, QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void chooseFiles();
    void startConversions(const QStringList &files);
    void conversionFinished(QVector<ConversionResult> results);

    ConversionService &m_conversionService;
    std::jthread m_worker;
    QLabel *m_dropHint = nullptr;
    bool m_converting = false;
};

} // namespace morpho::ui
