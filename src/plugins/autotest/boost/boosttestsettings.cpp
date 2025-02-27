// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "boosttestsettings.h"

#include "boosttestconstants.h"

#include "../autotestconstants.h"
#include "../autotesttr.h"

#include <utils/layoutbuilder.h>

using namespace Utils;

namespace Autotest {
namespace Internal {

BoostTestSettings::BoostTestSettings()
{
    setSettingsGroups("Autotest", "BoostTest");
    setAutoApply(false);

    registerAspect(&logLevel);
    logLevel.setSettingsKey("LogLevel");
    logLevel.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    logLevel.addOption("All");
    logLevel.addOption("Success");
    logLevel.addOption("Test Suite");
    logLevel.addOption("Unit Scope");
    logLevel.addOption("Message");
    logLevel.addOption("Warning");
    logLevel.addOption("Error");
    logLevel.addOption("C++ Exception");
    logLevel.addOption("System Error");
    logLevel.addOption("Fatal Error");
    logLevel.addOption("Nothing");
    logLevel.setDefaultValue(int(LogLevel::Warning));
    logLevel.setLabelText(Tr::tr("Log format:"));

    registerAspect(&reportLevel);
    reportLevel.setSettingsKey("ReportLevel");
    reportLevel.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    reportLevel.addOption("Confirm");
    reportLevel.addOption("Short");
    reportLevel.addOption("Detailed");
    reportLevel.addOption("No");
    reportLevel.setDefaultValue(int(ReportLevel::Confirm));
    reportLevel.setLabelText(Tr::tr("Report level:"));

    registerAspect(&seed);
    seed.setSettingsKey("Seed");
    seed.setEnabled(false);
    seed.setLabelText(Tr::tr("Seed:"));
    seed.setToolTip(Tr::tr("A seed of 0 means no randomization. A value of 1 uses the current "
        "time, any other value is used as random seed generator."));
    seed.setEnabler(&randomize);

    registerAspect(&randomize);
    randomize.setSettingsKey("Randomize");
    randomize.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    randomize.setLabelText(Tr::tr("Randomize"));
    randomize.setToolTip(Tr::tr("Randomize execution order."));

    registerAspect(&systemErrors);
    systemErrors.setSettingsKey("SystemErrors");
    systemErrors.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    systemErrors.setLabelText(Tr::tr("Catch system errors"));
    systemErrors.setToolTip(Tr::tr("Catch or ignore system errors."));

    registerAspect(&fpExceptions);
    fpExceptions.setSettingsKey("FPExceptions");
    fpExceptions.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    fpExceptions.setLabelText(Tr::tr("Floating point exceptions"));
    fpExceptions.setToolTip(Tr::tr("Enable floating point exception traps."));

    registerAspect(&memLeaks);
    memLeaks.setSettingsKey("MemoryLeaks");
    memLeaks.setLabelPlacement(BoolAspect::LabelPlacement::AtCheckBoxWithoutDummyLabel);
    memLeaks.setDefaultValue(true);
    memLeaks.setLabelText(Tr::tr("Detect memory leaks"));
    memLeaks.setToolTip(Tr::tr("Enable memory leak detection."));
}

BoostTestSettingsPage::BoostTestSettingsPage(BoostTestSettings *settings, Utils::Id settingsId)
{
    setId(settingsId);
    setCategory(Constants::AUTOTEST_SETTINGS_CATEGORY);
    setDisplayName(Tr::tr(BoostTest::Constants::FRAMEWORK_SETTINGS_CATEGORY));
    setSettings(settings);

    setLayouter([settings](QWidget *widget) {
        BoostTestSettings &s = *settings;
        using namespace Layouting;

        Grid grid {
            s.logLevel, br,
            s.reportLevel, br,
            s.randomize, Row { s.seed }, br,
            s.systemErrors, br,
            s.fpExceptions, br,
            s.memLeaks,
        };

        Column { Row { Column { grid, st }, st } }.attachTo(widget);
    });
}

QString BoostTestSettings::logLevelToOption(const LogLevel logLevel)
{
    switch (logLevel) {
    case LogLevel::All: return QString("all");
    case LogLevel::Success: return QString("success");
    case LogLevel::TestSuite: return QString("test_suite");
    case LogLevel::UnitScope: return QString("unit_scope");
    case LogLevel::Message: return QString("message");
    case LogLevel::Error: return QString("error");
    case LogLevel::CppException: return QString("cpp_exception");
    case LogLevel::SystemError: return QString("system_error");
    case LogLevel::FatalError: return QString("fatal_error");
    case LogLevel::Nothing: return QString("nothing");
    case LogLevel::Warning: return QString("warning");
    }
    return QString();
}

QString BoostTestSettings::reportLevelToOption(const ReportLevel reportLevel)
{
    switch (reportLevel) {
    case ReportLevel::Confirm: return QString("confirm");
    case ReportLevel::Short: return QString("short");
    case ReportLevel::Detailed: return QString("detailed");
    case ReportLevel::No: return QString("no");
    }
    return QString();
}

} // namespace Internal
} // namespace Autotest
