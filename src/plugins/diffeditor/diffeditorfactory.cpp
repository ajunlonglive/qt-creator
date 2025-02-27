// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include "diffeditorfactory.h"

#include "texteditor/texteditoractionhandler.h"

#include <QCoreApplication>

using namespace Core;
using namespace TextEditor;
using namespace Utils;

namespace DiffEditor {
namespace Internal {

DiffEditorFactory::DiffEditorFactory() :
    descriptionHandler {
        Constants::DIFF_EDITOR_ID,
        Constants::C_DIFF_EDITOR_DESCRIPTION,
        TextEditorActionHandler::None,
        [](IEditor *e) { return static_cast<DiffEditor *>(e)->descriptionWidget(); }
    },
    unifiedHandler {
        Constants::DIFF_EDITOR_ID,
        Constants::UNIFIED_VIEW_ID,
        TextEditorActionHandler::None,
        [](IEditor *e) { return static_cast<DiffEditor *>(e)->unifiedEditorWidget(); }
    },
    leftHandler {
        Constants::DIFF_EDITOR_ID,
        Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(1),
        TextEditorActionHandler::None,
        [](IEditor *e) { return static_cast<DiffEditor *>(e)->sideEditorWidget(LeftSide); }
    },
    rightHandler {
        Constants::DIFF_EDITOR_ID,
        Id(Constants::SIDE_BY_SIDE_VIEW_ID).withSuffix(2),
        TextEditorActionHandler::None,
        [](Core::IEditor *e) { return static_cast<DiffEditor *>(e)->sideEditorWidget(RightSide); }
    }
{
    setId(Constants::DIFF_EDITOR_ID);
    setDisplayName(QCoreApplication::translate("DiffEditorFactory", Constants::DIFF_EDITOR_DISPLAY_NAME));
    addMimeType(Constants::DIFF_EDITOR_MIMETYPE);
    setEditorCreator([] { return new DiffEditor(new DiffEditorDocument); });
}

} // namespace Internal
} // namespace DiffEditor
