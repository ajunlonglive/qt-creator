// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "uvscserverprovider.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
QT_END_NAMESPACE

namespace BareMetal::Internal {

// SimulatorUvscServerProvider

class SimulatorUvscServerProvider final : public UvscServerProvider
{
public:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const IDebugServerProvider &other) const final;
    bool isSimulator() const final { return true; }

    Utils::FilePath optionsFilePath(Debugger::DebuggerRunTool *runTool,
                                    QString &errorMessage) const final;

private:
    explicit SimulatorUvscServerProvider();

    bool m_limitSpeed = false;

    friend class SimulatorUvscServerProviderConfigWidget;
    friend class SimulatorUvscServerProviderFactory;
    friend class SimulatorUvProjectOptions;
};

// SimulatorUvscServerProviderFactory

class SimulatorUvscServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    SimulatorUvscServerProviderFactory();
};

// SimulatorUvscServerProviderConfigWidget

class SimulatorUvscServerProviderConfigWidget final : public UvscServerProviderConfigWidget
{
public:
    explicit SimulatorUvscServerProviderConfigWidget(SimulatorUvscServerProvider *provider);

private:
    void apply() override;
    void discard() override;

    void setFromProvider();

    QCheckBox *m_limitSpeedCheckBox = nullptr;
};

} // BareMetal::Internal
