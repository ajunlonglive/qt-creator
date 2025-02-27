// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>
#include "qmt/infrastructure/qmt_global.h"

#include <QColor>
#include <QVector>

namespace qmt {

class QMT_EXPORT PaletteBox : public QWidget
{
    Q_OBJECT

public:
    explicit PaletteBox(QWidget *parent = nullptr);
    ~PaletteBox() override;

signals:
    void activated(int index);

public:
    QBrush brush(int index) const;
    void setBrush(int index, const QBrush &brush);
    QPen linePen(int index) const;
    void setLinePen(int index, const QPen &pen);
    int currentIndex() const { return m_currentIndex; }

    void clear();
    void setCurrentIndex(int index);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QVector<QBrush> m_brushes;
    QVector<QPen> m_pens;
    int m_currentIndex = -1;
};

} // namespace qmt
