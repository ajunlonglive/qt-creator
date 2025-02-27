// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>

namespace Android {
namespace Internal {

class AndroidManifestEditorWidget;

class AndroidManifestDocument : public TextEditor::TextDocument
{
public:
    explicit AndroidManifestDocument(AndroidManifestEditorWidget *editorWidget);
    bool save(QString *errorString, const Utils::FilePath &filePath,
              bool autoSave = false) override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

private:
    AndroidManifestEditorWidget *m_editorWidget;
};

} // namespace Internal
} // namespace Android
