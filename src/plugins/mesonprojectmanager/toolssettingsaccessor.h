// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mesontools.h"

#include <utils/settingsaccessor.h>

namespace MesonProjectManager {
namespace Internal {

class ToolsSettingsAccessor final : public Utils::UpgradingSettingsAccessor
{
public:
    ToolsSettingsAccessor();
    void saveMesonTools(const std::vector<MesonTools::Tool_t> &tools, QWidget *parent);
    std::vector<MesonTools::Tool_t> loadMesonTools(QWidget *parent);
};

} // namespace Internal
} // namespace MesonProjectManager
