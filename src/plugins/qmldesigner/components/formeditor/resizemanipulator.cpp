// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "resizemanipulator.h"

#include "formeditoritem.h"
#include "formeditorscene.h"
#include "qmlanchors.h"
#include <QDebug>
#include "mathutils.h"

#include <limits>

namespace QmlDesigner {

ResizeManipulator::ResizeManipulator(LayerItem *layerItem, FormEditorView *view)
    : m_view(view),
    m_beginTopMargin(0.0),
    m_beginLeftMargin(0.0),
    m_beginRightMargin(0.0),
    m_beginBottomMargin(0.0),
    m_layerItem(layerItem),
    m_resizeHandle(nullptr),
    m_isActive(false)
{
}

ResizeManipulator::~ResizeManipulator()
{
    deleteSnapLines();
}

void ResizeManipulator::setHandle(ResizeHandleItem *resizeHandle)
{
    Q_ASSERT(resizeHandle);
    m_resizeHandle = resizeHandle;
    m_resizeController = resizeHandle->resizeController();
    m_snapper.setContainerFormEditorItem(m_resizeController.formEditorItem()->parentItem());
    m_snapper.setTransformtionSpaceFormEditorItem(m_resizeController.formEditorItem());
    Q_ASSERT(m_resizeController.isValid());
}

void ResizeManipulator::removeHandle()
{
    m_resizeController = ResizeController();
    m_resizeHandle = nullptr;
}

void ResizeManipulator::begin(const QPointF &/*beginPoint*/)
{
    if (m_resizeController.isValid()) {
        m_isActive = true;
        m_beginBoundingRect = m_resizeController.formEditorItem()->qmlItemNode().instanceBoundingRect();
        m_beginFromContentItemToSceneTransform = m_resizeController.formEditorItem()->instanceSceneContentItemTransform();
        m_beginFromSceneToContentItemTransform = m_beginFromContentItemToSceneTransform.inverted();
        m_beginFromItemToSceneTransform = m_resizeController.formEditorItem()->instanceSceneTransform();
        m_beginToParentTransform = m_resizeController.formEditorItem()->qmlItemNode().instanceTransform();
        m_rewriterTransaction = m_view->beginRewriterTransaction(QByteArrayLiteral("ResizeManipulator::begin"));
        m_rewriterTransaction.ignoreSemanticChecks();
        m_snapper.updateSnappingLines(m_resizeController.formEditorItem());
        m_beginBottomRightPoint = m_beginToParentTransform.map(m_resizeController.formEditorItem()->qmlItemNode().instanceBoundingRect().bottomRight());

        QmlAnchors anchors(m_resizeController.formEditorItem()->qmlItemNode().anchors());
        m_beginTopMargin = anchors.instanceMargin(AnchorLineTop);
        m_beginLeftMargin = anchors.instanceMargin(AnchorLineLeft);
        m_beginRightMargin = anchors.instanceMargin(AnchorLineRight);
        m_beginBottomMargin = anchors.instanceMargin(AnchorLineBottom);
    }
}

//static QSizeF mapSizeToParent(const QSizeF &size, QGraphicsItem *item)
//{
//    QPointF sizeAsPoint(size.width(), size.height());
//    sizeAsPoint = item->mapToParent(sizeAsPoint);
//    return QSizeF(sizeAsPoint.x(), sizeAsPoint.y());
//}

void ResizeManipulator::update(const QPointF& updatePoint, Snapper::Snapping useSnapping, Qt::KeyboardModifiers keyMods)
{
    const bool preserveAspectRatio = keyMods.testFlag(Qt::ShiftModifier);
    const bool resizeFromCenter = keyMods.testFlag(Qt::AltModifier);

    const double minimumWidth = 0.0;
    const double minimumHeight = 0.0;

    deleteSnapLines();

    bool snap = useSnapping == Snapper::UseSnapping || useSnapping == Snapper::UseSnappingAndAnchoring;

    if (m_resizeController.isValid()) {

        FormEditorItem *formEditorItem = m_resizeController.formEditorItem();
        FormEditorItem *containerItem = m_snapper.containerFormEditorItem();

        if (!containerItem)
            return;

        QPointF updatePointInLocalSpace = m_beginFromSceneToContentItemTransform.map(updatePoint);
        QmlAnchors anchors(formEditorItem->qmlItemNode().anchors());

        QRectF boundingRect(m_beginBoundingRect);

        auto getRatioSizes = [&](){
            double ratio = std::min(boundingRect.width() / m_beginBoundingRect.width(),
                                    boundingRect.height() / m_beginBoundingRect.height());
            double newW = m_beginBoundingRect.width() * ratio;
            double newH = m_beginBoundingRect.height() * ratio;

            return QSizeF(newW, newH);
        };

        if (m_resizeHandle->isBottomRightHandle()) {
            boundingRect.setBottomRight(updatePointInLocalSpace);

            if (snap) {
                double rightOffset = m_snapper.snapRightOffset(boundingRect);
                if (rightOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= rightOffset;

                double bottomOffset = m_snapper.snapBottomOffset(boundingRect);
                if (bottomOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= bottomOffset;
            }
            boundingRect.setBottomRight(updatePointInLocalSpace);

            if (preserveAspectRatio) {
                QSizeF newSize = getRatioSizes();

                updatePointInLocalSpace.rx() = (boundingRect.topLeft().x() + newSize.width());
                updatePointInLocalSpace.ry() = (boundingRect.topLeft().y() + newSize.height());

                boundingRect.setBottomRight(updatePointInLocalSpace);
            }

            if (resizeFromCenter) {
                QPointF grow = { boundingRect.width() - m_beginBoundingRect.width(),
                         boundingRect.height() - m_beginBoundingRect.height() };

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineTop))
                        boundingRect.setTop(boundingRect.top() - grow.y());
                    if (!anchors.instanceHasAnchor(AnchorLineLeft))
                        boundingRect.setLeft(boundingRect.left() - grow.x());
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                boundingRect.setLeft(boundingRect.left() - (updatePointInLocalSpace.x() - m_beginBoundingRect.right()));

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                boundingRect.setTop(boundingRect.top() - (updatePointInLocalSpace.y() - m_beginBoundingRect.bottom()));

            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);
            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLineBottom)) {
               anchors.setMargin(AnchorLineBottom,
                       m_beginBottomMargin - (m_beginToParentTransform.map(boundingRect.bottomRight())  - m_beginBottomRightPoint).y());
            }

            if (anchors.instanceHasAnchor(AnchorLineRight)) {
               anchors.setMargin(AnchorLineRight,
                       m_beginRightMargin - (m_beginToParentTransform.map(boundingRect.bottomRight()) - m_beginBottomRightPoint).x());
            }
        } else if (m_resizeHandle->isTopLeftHandle()) {
            boundingRect.setTopLeft(updatePointInLocalSpace);

            if (snap) {
                double leftOffset = m_snapper.snapLeftOffset(boundingRect);
                if (leftOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= leftOffset;

                double topOffset = m_snapper.snapTopOffset(boundingRect);
                if (topOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= topOffset;
            }
            boundingRect.setTopLeft(updatePointInLocalSpace);

            if (preserveAspectRatio) {
                QSizeF newSize = getRatioSizes();

                updatePointInLocalSpace.rx() = (boundingRect.bottomRight().x() - newSize.width());
                updatePointInLocalSpace.ry() = (boundingRect.bottomRight().y() - newSize.height());

                boundingRect.setTopLeft(updatePointInLocalSpace);
            }

            if (resizeFromCenter) {
                QPointF grow = { boundingRect.width() - m_beginBoundingRect.width(),
                         boundingRect.height() - m_beginBoundingRect.height() };

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineBottom))
                        boundingRect.setBottom(boundingRect.bottom() + grow.y());
                    if (!anchors.instanceHasAnchor(AnchorLineRight))
                        boundingRect.setRight(boundingRect.right() + grow.x());
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                boundingRect.setRight(boundingRect.right() - (updatePointInLocalSpace.x() - m_beginBoundingRect.left()));

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                boundingRect.setBottom(boundingRect.bottom() - (updatePointInLocalSpace.y() - m_beginBoundingRect.top()));


            if (boundingRect.width() < minimumWidth)
                boundingRect.setLeft(boundingRect.left() - minimumWidth + boundingRect.width());
            if (boundingRect.height() < minimumHeight)
                boundingRect.setTop(boundingRect.top() - minimumHeight + boundingRect.height());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));


            if (anchors.instanceHasAnchor(AnchorLineTop)) {
               anchors.setMargin(AnchorLineTop,
                       m_beginTopMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).y() + m_beginToParentTransform.map(boundingRect.topLeft()).y()));
            }

            if (anchors.instanceHasAnchor(AnchorLineLeft)) {
               anchors.setMargin(AnchorLineLeft,
                       m_beginLeftMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).x() + m_beginToParentTransform.map(boundingRect.topLeft()).x()));
            }

        } else if (m_resizeHandle->isTopRightHandle()) {
            boundingRect.setTopRight(updatePointInLocalSpace);

            if (snap) {
                double rightOffset = m_snapper.snapRightOffset(boundingRect);
                if (rightOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= rightOffset;

                double topOffset = m_snapper.snapTopOffset(boundingRect);
                if (topOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= topOffset;
            }
            boundingRect.setTopRight(updatePointInLocalSpace);

            if (preserveAspectRatio) {
                QSizeF newSize = getRatioSizes();

                updatePointInLocalSpace.rx() = (boundingRect.bottomLeft().x() + newSize.width());
                updatePointInLocalSpace.ry() = (boundingRect.bottomLeft().y() - newSize.height());

                boundingRect.setTopRight(updatePointInLocalSpace);
            }

            if (resizeFromCenter) {
                QPointF grow = { boundingRect.width() - m_beginBoundingRect.width(),
                         boundingRect.height() - m_beginBoundingRect.height() };

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineBottom))
                        boundingRect.setBottom(boundingRect.bottom() + grow.y());
                    if (!anchors.instanceHasAnchor(AnchorLineLeft))
                        boundingRect.setLeft(boundingRect.left() - grow.x());
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                boundingRect.setLeft(boundingRect.left() - (updatePointInLocalSpace.x() - m_beginBoundingRect.right()));

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                boundingRect.setBottom(boundingRect.bottom() - (updatePointInLocalSpace.y() - m_beginBoundingRect.top()));

            if (boundingRect.height() < minimumHeight)
                boundingRect.setTop(boundingRect.top() - minimumHeight + boundingRect.height());
            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition( m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLineTop)) {
               anchors.setMargin(AnchorLineTop,
                       m_beginTopMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).y() + m_beginToParentTransform.map(boundingRect.topLeft()).y()));
            }

            if (anchors.instanceHasAnchor(AnchorLineRight)) {
               anchors.setMargin(AnchorLineRight,
                       m_beginRightMargin - (m_beginToParentTransform.map(boundingRect.bottomRight()) - m_beginBottomRightPoint).x());
            }
        } else if (m_resizeHandle->isBottomLeftHandle()) {
            boundingRect.setBottomLeft(updatePointInLocalSpace);

            if (snap) {
                double leftOffset = m_snapper.snapLeftOffset(boundingRect);
                if (leftOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= leftOffset;

                double bottomOffset = m_snapper.snapBottomOffset(boundingRect);
                if (bottomOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= bottomOffset;
            }

            boundingRect.setBottomLeft(updatePointInLocalSpace);

            if (preserveAspectRatio) {
                QSizeF newSize = getRatioSizes();

                updatePointInLocalSpace.rx() = (boundingRect.topRight().x() - newSize.width());
                updatePointInLocalSpace.ry() = (boundingRect.topRight().y() + newSize.height());

                boundingRect.setBottomLeft(updatePointInLocalSpace);
            }

            if (resizeFromCenter) {
                QPointF grow = { boundingRect.width() - m_beginBoundingRect.width(),
                         boundingRect.height() - m_beginBoundingRect.height() };

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineTop))
                        boundingRect.setTop(boundingRect.top() - grow.y());
                    if (!anchors.instanceHasAnchor(AnchorLineRight))
                        boundingRect.setRight(boundingRect.right() + grow.x());
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                boundingRect.setRight(boundingRect.right() - (updatePointInLocalSpace.x() - m_beginBoundingRect.left()));

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                boundingRect.setTop(boundingRect.top() - (updatePointInLocalSpace.y() - m_beginBoundingRect.bottom()));

            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);
            if (boundingRect.width() < minimumWidth)
                boundingRect.setLeft(boundingRect.left() - minimumWidth + boundingRect.width());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLineLeft)) {
               anchors.setMargin(AnchorLineLeft,
                       m_beginLeftMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).x() + m_beginToParentTransform.map(boundingRect.topLeft()).x()));
            }

            if (anchors.instanceHasAnchor(AnchorLineBottom)) {
               anchors.setMargin(AnchorLineBottom,
                       m_beginBottomMargin - (m_beginToParentTransform.map(boundingRect.bottomRight())  - m_beginBottomRightPoint).y());
            }
        } else if (m_resizeHandle->isBottomHandle()) {
            boundingRect.setBottom(updatePointInLocalSpace.y());

            if (snap) {
                double bottomOffset = m_snapper.snapBottomOffset(boundingRect);
                if (bottomOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= bottomOffset;
            }

            boundingRect.setBottom(updatePointInLocalSpace.y());

            if (resizeFromCenter) {
                double grow = boundingRect.height() - m_beginBoundingRect.height();

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineTop))
                        boundingRect.setTop(boundingRect.top() - grow);
                    if (!anchors.instanceHasAnchor(AnchorLineLeft))
                        boundingRect.setLeft(boundingRect.left() - grow);
                    if (!anchors.instanceHasAnchor(AnchorLineRight))
                        boundingRect.setRight(boundingRect.right() + grow);
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                boundingRect.setTop(boundingRect.top() - (updatePointInLocalSpace.y() - m_beginBoundingRect.bottom()));

            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);
            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLineBottom)) {
               anchors.setMargin(AnchorLineBottom,
                       m_beginBottomMargin - (m_beginToParentTransform.map(boundingRect.bottomRight())  - m_beginBottomRightPoint).y());
            }
        } else if (m_resizeHandle->isTopHandle()) {
            boundingRect.setTop(updatePointInLocalSpace.y());

            if (snap) {
                double topOffset = m_snapper.snapTopOffset(boundingRect);
                if (topOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.ry() -= topOffset;
            }

            boundingRect.setTop(updatePointInLocalSpace.y());

            if (resizeFromCenter) {
                double grow = boundingRect.height() - m_beginBoundingRect.height();

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineBottom))
                        boundingRect.setBottom(boundingRect.bottom() + grow);
                    if (!anchors.instanceHasAnchor(AnchorLineLeft))
                        boundingRect.setLeft(boundingRect.left() - grow);
                    if (!anchors.instanceHasAnchor(AnchorLineRight))
                        boundingRect.setRight(boundingRect.right() + grow);
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                boundingRect.setBottom(boundingRect.bottom() - (updatePointInLocalSpace.y() - m_beginBoundingRect.top()));

            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);
            if (boundingRect.height() < minimumHeight)
                boundingRect.setTop(boundingRect.top() - minimumHeight + boundingRect.height());

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLineTop)) {
               anchors.setMargin(AnchorLineTop,
                       m_beginTopMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).y() + m_beginToParentTransform.map(boundingRect.topLeft()).y()));
            }
        } else if (m_resizeHandle->isRightHandle()) {
            boundingRect.setRight(updatePointInLocalSpace.x());

            if (snap) {
                double rightOffset = m_snapper.snapRightOffset(boundingRect);
                if (rightOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= rightOffset;
            }

            boundingRect.setRight(updatePointInLocalSpace.x());

            if (resizeFromCenter) {
                double grow = boundingRect.width() - m_beginBoundingRect.width();

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineTop))
                        boundingRect.setTop(boundingRect.top() - grow);
                    if (!anchors.instanceHasAnchor(AnchorLineLeft))
                        boundingRect.setLeft(boundingRect.left() - grow);
                    if (!anchors.instanceHasAnchor(AnchorLineBottom))
                        boundingRect.setBottom(boundingRect.bottom() + grow);
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                boundingRect.setLeft(boundingRect.left() - (updatePointInLocalSpace.x() - m_beginBoundingRect.right()));

            if (boundingRect.width() < minimumWidth)
                boundingRect.setWidth(minimumWidth);
            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));


            if (anchors.instanceHasAnchor(AnchorLineRight)) {
               anchors.setMargin(AnchorLineRight,
                       m_beginRightMargin - (m_beginToParentTransform.map(boundingRect.bottomRight()) - m_beginBottomRightPoint).x());
            }
        } else if (m_resizeHandle->isLeftHandle()) {
            boundingRect.setLeft(updatePointInLocalSpace.x());

            if (snap) {
                double leftOffset = m_snapper.snapLeftOffset(boundingRect);
                if (leftOffset < std::numeric_limits<double>::max())
                    updatePointInLocalSpace.rx() -= leftOffset;
            }

            boundingRect.setLeft(updatePointInLocalSpace.x());

            if (resizeFromCenter) {
                double grow = boundingRect.width() - m_beginBoundingRect.width();

                if (!anchors.instanceHasAnchor(AnchorLineHorizontalCenter)
                        && !anchors.instanceHasAnchor(AnchorLineVerticalCenter)) {
                    if (!anchors.instanceHasAnchor(AnchorLineTop))
                        boundingRect.setTop(boundingRect.top() - grow);
                    if (!anchors.instanceHasAnchor(AnchorLineBottom))
                        boundingRect.setBottom(boundingRect.bottom() + grow);
                    if (!anchors.instanceHasAnchor(AnchorLineRight))
                        boundingRect.setRight(boundingRect.right() + grow);
                }
            }

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                boundingRect.setRight(boundingRect.right() - (updatePointInLocalSpace.x() - m_beginBoundingRect.left()));

            if (boundingRect.width() < minimumWidth)
                boundingRect.setLeft(boundingRect.left() - minimumWidth + boundingRect.width());
            if (boundingRect.height() < minimumHeight)
                boundingRect.setHeight(minimumHeight);

            formEditorItem->qmlItemNode().setSize(boundingRect.size());
            formEditorItem->qmlItemNode().setPosition(m_beginToParentTransform.map(boundingRect.topLeft()));

            if (anchors.instanceHasAnchor(AnchorLineLeft)) {
               anchors.setMargin(AnchorLineLeft,
                       m_beginLeftMargin + (-m_beginToParentTransform.map(m_beginBoundingRect.topLeft()).x() + m_beginToParentTransform.map(boundingRect.topLeft()).x()));
            }
        }

        if (snap)
            m_graphicsLineList = m_snapper.generateSnappingLines(boundingRect,
                                                                 m_layerItem.data(),
                                                                 m_beginFromItemToSceneTransform);
    }
}

void ResizeManipulator::end(Snapper::Snapping useSnapping)
{
    if (useSnapping == Snapper::UseSnappingAndAnchoring) {
        deleteSnapLines();
        m_snapper.setTransformtionSpaceFormEditorItem(m_snapper.containerFormEditorItem());
        m_snapper.updateSnappingLines(m_resizeController.formEditorItem());
        m_snapper.adjustAnchoringOfItem(m_resizeController.formEditorItem());
    }

    m_isActive = false;
    m_rewriterTransaction.commit();
    clear();
    removeHandle();
}

void ResizeManipulator::moveBy(double deltaX, double deltaY)
{
    if (resizeHandle() && m_resizeController.isValid()) {
        QmlItemNode qmlItemNode(m_resizeController.formEditorItem()->qmlItemNode());
        QmlAnchors anchors(qmlItemNode.anchors());

        if (m_resizeController.isLeftHandle(resizeHandle())
            || m_resizeController.isTopLeftHandle(resizeHandle())
            || m_resizeController.isBottomLeftHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("x", round((qmlItemNode.instanceValue("x").toDouble() + deltaX), 4));
            qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() - deltaX, 4));


            if (anchors.instanceHasAnchor(AnchorLineLeft))
               anchors.setMargin(AnchorLineLeft, anchors.instanceMargin(AnchorLineLeft) + deltaX);

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() - (deltaX * 2), 4));
        }

        if (m_resizeController.isRightHandle(resizeHandle())
            || m_resizeController.isTopRightHandle(resizeHandle())
            || m_resizeController.isBottomRightHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() + deltaX, 4));

            if (anchors.instanceHasAnchor(AnchorLineRight))
               anchors.setMargin(AnchorLineRight, round(anchors.instanceMargin(AnchorLineRight) - deltaX, 4));

            if (anchors.instanceHasAnchor(AnchorLineHorizontalCenter))
                qmlItemNode.setVariantProperty("width", round(qmlItemNode.instanceValue("width").toDouble() + (deltaX * 2), 4));
        }


        if (m_resizeController.isTopHandle(resizeHandle())
            || m_resizeController.isTopLeftHandle(resizeHandle())
            || m_resizeController.isTopRightHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("y", round(qmlItemNode.instanceValue("y").toDouble() + deltaY, 4));
            qmlItemNode.setVariantProperty("height", round(qmlItemNode.instanceValue("height").toDouble() - deltaY, 4));

            if (anchors.instanceHasAnchor(AnchorLineTop))
               anchors.setMargin(AnchorLineTop, anchors.instanceMargin(AnchorLineTop) + deltaY);

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                qmlItemNode.setVariantProperty("height", round(qmlItemNode.instanceValue("height").toDouble() - (deltaY * 2), 4));
        }

        if (m_resizeController.isBottomHandle(resizeHandle())
            || m_resizeController.isBottomLeftHandle(resizeHandle())
            || m_resizeController.isBottomRightHandle(resizeHandle())) {
            qmlItemNode.setVariantProperty("height",  round(qmlItemNode.instanceValue("height").toDouble() + deltaY, 4));

            if (anchors.instanceHasAnchor(AnchorLineBottom))
               anchors.setMargin(AnchorLineBottom, anchors.instanceMargin(AnchorLineBottom) - deltaY);

            if (anchors.instanceHasAnchor(AnchorLineVerticalCenter))
                qmlItemNode.setVariantProperty("height",  round(qmlItemNode.instanceValue("height").toDouble() + (deltaY * 2), 4));
        }

    }
}

bool ResizeManipulator::isInvalidSize(const QSizeF & size)
{
    if (size.width() < 0 || size.height() < 0)
        return true;

    return false;
}

void ResizeManipulator::deleteSnapLines()
{
    if (m_layerItem) {
        for (QGraphicsItem *item : std::as_const(m_graphicsLineList)) {
            m_layerItem->scene()->removeItem(item);
            delete item;
        }
    }

    m_graphicsLineList.clear();
    m_view->scene()->update();
}

ResizeHandleItem *ResizeManipulator::resizeHandle()
{
    return m_resizeHandle;
}

void ResizeManipulator::clear()
{
    m_rewriterTransaction.commit();

    deleteSnapLines();
    m_beginBoundingRect = QRectF();
    m_beginFromSceneToContentItemTransform = QTransform();
    m_beginFromContentItemToSceneTransform = QTransform();
    m_beginFromItemToSceneTransform = QTransform();
    m_beginToParentTransform = QTransform();
    m_beginTopMargin = 0.0;
    m_beginLeftMargin = 0.0;
    m_beginRightMargin = 0.0;
    m_beginBottomMargin = 0.0;
    removeHandle();
}

bool ResizeManipulator::isActive() const
{
    return m_isActive;
}

}
