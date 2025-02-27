// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "testresultmodel.h"

#include <QStyledItemDelegate>
#include <QTextLayout>

namespace Autotest {
namespace Internal {

class TestResultDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TestResultDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);
    void clearCache();

private:
    void limitTextOutput(QString &output) const;
    void recalculateTextLayout(const QModelIndex &index, const QString &output,
                               const QFont &font, int width) const;

    mutable QModelIndex m_lastProcessedIndex;
    mutable QFont m_lastProcessedFont;
    mutable QTextLayout m_lastCalculatedLayout;
    mutable int m_lastCalculatedHeight;
    mutable int m_lastWidth = -1;

    class LayoutPositions
    {
    public:
        LayoutPositions(QStyleOptionViewItem &options, const TestResultFilterModel *filterModel)
            : m_top(options.rect.top()),
              m_bottom(options.rect.bottom()),
              m_left(options.rect.left()),
              m_right(options.rect.right())
        {
            TestResultModel *srcModel = static_cast<TestResultModel *>(filterModel->sourceModel());
            m_maxFileLength = srcModel->maxWidthOfFileName(options.font);
            m_maxLineLength = srcModel->maxWidthOfLineNumber(options.font);
            m_realFileLength = m_maxFileLength;
            m_typeAreaWidth = QFontMetrics(options.font).horizontalAdvance("XXXXXXXX");

            int flexibleArea = lineAreaLeft() - textAreaLeft() - ITEM_SPACING;
            if (m_maxFileLength > flexibleArea / 2)
                m_realFileLength = flexibleArea / 2;
            m_fontHeight = QFontMetrics(options.font).height();
        }

        int top() const { return m_top + ITEM_MARGIN; }
        int left() const { return m_left + ITEM_MARGIN; }
        int right() const { return m_right - ITEM_MARGIN; }
        int bottom() const { return m_bottom; }
        int minimumHeight() const { return ICON_SIZE + 2 * ITEM_MARGIN; }

        int iconSize() const { return ICON_SIZE; }
        int fontHeight() const { return m_fontHeight; }
        int typeAreaLeft() const { return left() + ICON_SIZE + ITEM_SPACING; }
        int typeAreaWidth() const { return m_typeAreaWidth; }
        int textAreaLeft() const { return typeAreaLeft() + m_typeAreaWidth + ITEM_SPACING; }
        int textAreaWidth() const { return fileAreaLeft() - ITEM_SPACING - textAreaLeft(); }
        int fileAreaLeft() const { return lineAreaLeft() - ITEM_SPACING - m_realFileLength; }
        int lineAreaLeft() const { return right() - m_maxLineLength; }

        QRect typeArea() const { return QRect(typeAreaLeft(), top(),
                                              typeAreaWidth(), m_fontHeight); }
        QRect textArea() const { return QRect(textAreaLeft(), top(),
                                              textAreaWidth(), m_fontHeight); }
        QRect fileArea() const { return QRect(fileAreaLeft(), top(),
                                              m_realFileLength + ITEM_SPACING, m_fontHeight); }

        QRect lineArea() const { return QRect(lineAreaLeft(), top(),
                                              m_maxLineLength, m_fontHeight); }

    private:
        int m_maxFileLength;
        int m_maxLineLength;
        int m_realFileLength;
        int m_top;
        int m_bottom;
        int m_left;
        int m_right;
        int m_fontHeight;
        int m_typeAreaWidth;

        static const int ICON_SIZE = 16;
        static const int ITEM_MARGIN = 2;
        static const int ITEM_SPACING = 4;

    };
};

} // namespace Internal
} // namespace Autotest
