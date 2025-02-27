// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QFileInfo;
QT_END_NAMESPACE

namespace QmlDesigner {

class ContentLibraryTexture;

class ContentLibraryTexturesCategory : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString bundleCategoryName MEMBER m_name CONSTANT)
    Q_PROPERTY(bool bundleCategoryVisible MEMBER m_visible NOTIFY categoryVisibleChanged)
    Q_PROPERTY(bool bundleCategoryExpanded MEMBER m_expanded NOTIFY categoryExpandChanged)
    Q_PROPERTY(QList<ContentLibraryTexture *> bundleCategoryTextures MEMBER m_categoryTextures
               NOTIFY bundleTexturesModelChanged)

public:
    ContentLibraryTexturesCategory(QObject *parent, const QString &name);

    void addTexture(const QFileInfo &tex);
    bool filter(const QString &searchText);

    QString name() const;
    bool visible() const;
    bool expanded() const;
    QList<ContentLibraryTexture *> categoryTextures() const;

signals:
    void categoryVisibleChanged();
    void categoryExpandChanged();
    void bundleTexturesModelChanged();

private:
    QString m_name;
    bool m_visible = true;
    bool m_expanded = true;

    QList<ContentLibraryTexture *> m_categoryTextures;
};

} // namespace QmlDesigner
