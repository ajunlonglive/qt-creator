// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprofilerconstants.h"
#include "qmlprofilerplugin.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilersettings.h"
#include "qmlprofilertr.h"

#include <debugger/analyzer/analyzerrunconfigwidget.h>

namespace QmlProfiler {
namespace Internal {

QmlProfilerRunConfigurationAspect::QmlProfilerRunConfigurationAspect(ProjectExplorer::Target *)
{
    setProjectSettings(new QmlProfilerSettings);
    setGlobalSettings(QmlProfilerPlugin::globalSettings());
    setId(Constants::SETTINGS);
    setDisplayName(Tr::tr("QML Profiler Settings"));
    setUsingGlobalSettings(true);
    resetProjectToGlobalSettings();
    setConfigWidgetCreator([this] { return new Debugger::AnalyzerRunConfigWidget(this); });
}

} // Internal
} // QmlProfiler
