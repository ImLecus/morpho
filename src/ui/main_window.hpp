// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Morpho contributors

#pragma once

#include <morpho/core/conversion.hpp>

#include <QByteArray>
#include <QMainWindow>
#include <QStringList>
#include <QVector>

#include <thread>

class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;
class QEvent;
class QLabel;
class QScrollArea;
class QVBoxLayout;

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
    void addConvertedFile(ConversionResult result);
    bool saveConvertedFile(const QByteArray &bytes, const QString &fileName);

    ConversionService &m_conversionService;
    std::jthread m_worker;
    QLabel *m_dropHint = nullptr;
    QScrollArea *m_convertedScroll = nullptr;
    QWidget *m_convertedContainer = nullptr;
    QVBoxLayout *m_convertedLayout = nullptr;
    bool m_converting = false;
};

} // namespace morpho::ui
