// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cpptoolstestcase.h"

#include <texteditor/commentssettings.h>

#include <QObject>
#include <QScopedPointer>

namespace CppEditor {
namespace Internal {
namespace Tests {

/// Tests for inserting doxygen comments.
class DoxygenTest: public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanTestCase();
    void cleanup();

    void testBasic_data();
    void testBasic();

    void testWithMacroFromHeaderBeforeFunction();

    void testNoLeadingAsterisks_data();
    void testNoLeadingAsterisks();

private:
    void verifyCleanState() const;
    void runTest(const QByteArray &original,
                 const QByteArray &expected,
                 TextEditor::CommentsSettings *settings = 0,
                 const TestDocuments &includedHeaderDocuments = TestDocuments());

    QScopedPointer<TextEditor::CommentsSettings> oldSettings;
};

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
