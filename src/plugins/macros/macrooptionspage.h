// Copyright (C) 2016 Nicolas Arnaud-Cormos
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>

namespace Macros {
namespace Internal {

class MacroOptionsPage final : public Core::IOptionsPage
{
public:
    MacroOptionsPage();
};

} // namespace Internal
} // namespace Macros
