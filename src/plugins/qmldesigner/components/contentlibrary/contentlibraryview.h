// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "abstractview.h"
#include "nodemetainfo.h"

#include <QObject>
#include <QPointer>

namespace QmlDesigner {

class ContentLibraryMaterial;
class ContentLibraryWidget;
class Model;

class ContentLibraryView : public AbstractView
{
    Q_OBJECT

public:
    ContentLibraryView(ExternalDependenciesInterface &externalDependencies);
    ~ContentLibraryView() override;

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void active3DSceneChanged(qint32 sceneId) override;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;

private:
    void updateBundleMaterialsImportedState();
    void updateBundleMaterialsQuick3DVersion();
    void applyBundleMaterialToDropTarget(const ModelNode &bundleMat, const NodeMetaInfo &metaInfo = {});
    ModelNode getBundleMaterialDefaultInstance(const TypeName &type);
    ModelNode createMaterial(const NodeMetaInfo &metaInfo);

    QPointer<ContentLibraryWidget> m_widget;
    QList<ModelNode> m_bundleMaterialTargets;
    QList<ModelNode> m_selectedModels; // selected 3D model nodes
    ContentLibraryMaterial *m_draggedBundleMaterial = nullptr;
    ModelNode m_activeSceneEnv;
    bool m_bundleMaterialAddToSelected = false;
    bool m_hasQuick3DImport = false;
};

} // namespace QmlDesigner
