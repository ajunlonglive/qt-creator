// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "iassistproposal.h"

using namespace TextEditor;

/*!
    \group CodeAssist
    \title Code Assist for Editors

    Code assist is available in the form of completions and refactoring actions pop-ups
    which are triggered under particular circumstances. This group contains the classes
    used to provide such support.

    Completions can be of a variety of kind like function hints, snippets, and regular
    context-aware content. The later are usually represented by semantic proposals, but
    it is also possible that they are simply plain text like in the fake vim mode.

    Completions also have the possibility to run asynchronously in a separate thread and
    then not blocking the GUI. This is the default behavior.
*/

/*!
    \class TextEditor::IAssistProposal
    \brief The IAssistProposal class acts as an interface for representing an assist proposal.
    \ingroup CodeAssist

    Known implenters of this interface are FunctionHintProposal and GenericProposal. The
    former is recommended to be used when assisting function call constructs (overloads
    and parameters) while the latter is quite generic so that it could be used to propose
    snippets, refactoring operations (quickfixes), and contextual content (the member of
    class or a string existent in the document, for example).

    This class is part of the CodeAssist API.

    \sa IAssistProposalWidget, IAssistModel
*/

IAssistProposal::IAssistProposal(Utils::Id id, int basePosition)
    : m_id(id)
    , m_basePosition(basePosition)
{}

IAssistProposal::~IAssistProposal() = default;

/*!
    \fn bool TextEditor::IAssistProposal::isFragile() const

    Returns whether this is a fragile proposal. When a proposal is fragile it means that
    it will be replaced by a new proposal in the case one is created, even if due to an
    idle editor.
*/

/*!
    \fn int TextEditor::IAssistProposal::basePosition() const

    Returns the position from which this proposal starts.
*/

int IAssistProposal::basePosition() const
{
    return m_basePosition;
}

bool IAssistProposal::isFragile() const
{
    return m_isFragile;
}

bool IAssistProposal::supportsPrefixFiltering(const QString &prefix) const
{
    return !m_prefixChecker || m_prefixChecker(prefix);
}

/*!
    \fn bool TextEditor::IAssistProposal::isCorrective() const

    Returns whether this proposal is also corrective. This could happen in C++, for example,
    when a dot operator (.) needs to be replaced by an arrow operator (->) before the proposal
    is displayed.
*/

bool IAssistProposal::isCorrective(TextEditorWidget *editorWidget) const
{
    Q_UNUSED(editorWidget)
    return false;
}

/*!
    \fn void TextEditor::IAssistProposal::makeCorrection(BaseTextEditor *editor)

    This allows a correction to be made in the case this is a corrective proposal.
*/

void IAssistProposal::makeCorrection(TextEditorWidget *editorWidget)
{
    Q_UNUSED(editorWidget)
}

void IAssistProposal::setFragile(bool fragile)
{
    m_isFragile = fragile;
}

void IAssistProposal::setPrefixChecker(const PrefixChecker checker)
{
    m_prefixChecker = checker;
}

/*!
    \fn IAssistModel *TextEditor::IAssistProposal::model() const

    Returns the model associated with this proposal.

    Although the IAssistModel from this proposal may be used on its own, it needs to be
    consistent with the widget returned by createWidget().

    \sa createWidget()
*/

/*!
    \fn IAssistProposalWidget *TextEditor::IAssistProposal::createWidget() const

    Returns the widget associated with this proposal. The IAssistProposalWidget implementor
    recommended for function hint proposals is FunctionHintProposalWidget. For snippets,
    refactoring operations (quickfixes), and contextual content the recommeded implementor
    is GenericProposalWidget.
*/
