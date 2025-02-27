// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#include "assetexportdialog.h"

#include "ui_assetexportdialog.h"
#include "assetexportpluginconstants.h"
#include "filepathmodel.h"

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <projectexplorer/task.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/project.h>
#include <projectexplorer/session.h>
#include <utils/fileutils.h>
#include <utils/outputformatter.h>

#include <QCheckBox>
#include <QPushButton>
#include <QListView>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QScrollBar>

#include <algorithm>
#include <cmath>

namespace  {
static void addFormattedMessage(Utils::OutputFormatter *formatter, const QString &str,
                                Utils::OutputFormat format) {
    if (!formatter)
        return;

    QPlainTextEdit *edit = formatter->plainTextEdit();
    QScrollBar *scroll = edit->verticalScrollBar();
    bool isAtBottom =  scroll && scroll->value() == scroll->maximum();

    QString msg = str + "\n";
    formatter->appendMessage(msg, format);

    if (isAtBottom)
        scroll->setValue(scroll->maximum());
}
}

using namespace ProjectExplorer;

namespace QmlDesigner {
AssetExportDialog::AssetExportDialog(const Utils::FilePath &exportPath,
                                     AssetExporter &assetExporter, FilePathModel &model,
                                     QWidget *parent) :
    QDialog(parent),
    m_assetExporter(assetExporter),
    m_filePathModel(model),
    m_ui(new Ui::AssetExportDialog),
    m_filesView(new QListView),
    m_exportLogs(new QPlainTextEdit),
    m_outputFormatter(new Utils::OutputFormatter())
{
    m_ui->setupUi(this);

    m_ui->exportPath->setExpectedKind(Utils::PathChooser::Kind::SaveFile);
    m_ui->exportPath->setFilePath(
                exportPath.pathAppended(
                    ProjectExplorer::SessionManager::startupProject()->displayName()  + ".metadata"
                ));
    m_ui->exportPath->setPromptDialogTitle(tr("Choose Export File"));
    m_ui->exportPath->setPromptDialogFilter(tr("Metadata file (*.metadata)"));
    m_ui->exportPath->lineEdit()->setReadOnly(true);
    m_ui->exportPath->addButton(tr("Open"), this, [this]() {
        Core::FileUtils::showInGraphicalShell(Core::ICore::dialogParent(), m_ui->exportPath->filePath());
    });

    auto optionsWidget = new QWidget;
    m_ui->advancedOptions->setSummaryText(tr("Advanced Options"));
    m_ui->advancedOptions->setWidget(optionsWidget);
    auto optionsLayout = new QHBoxLayout(optionsWidget);
    optionsLayout->setContentsMargins(8, 8, 8, 8);

    m_exportAssetsCheck = new QCheckBox(tr("Export assets"), this);
    m_exportAssetsCheck->setChecked(true);
    optionsLayout->addWidget(m_exportAssetsCheck);

    m_perComponentExportCheck = new QCheckBox(tr("Export components separately"), this);
    m_perComponentExportCheck->setChecked(false);
    optionsLayout->addWidget(m_perComponentExportCheck);
    optionsLayout->addStretch();

    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);

    m_ui->stackedWidget->addWidget(m_filesView);
    m_filesView->setModel(&m_filePathModel);

    m_exportLogs->setReadOnly(true);
    m_outputFormatter->setPlainTextEdit(m_exportLogs);
    m_ui->stackedWidget->addWidget(m_exportLogs);
    switchView(false);

    connect(m_ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked, [this]() {
        m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
        m_assetExporter.cancel();
    });

    m_exportBtn = m_ui->buttonBox->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &AssetExportDialog::onExport);
    connect(&m_filePathModel, &FilePathModel::modelReset, this, [this]() {
        m_ui->exportProgress->setRange(0, 1000);
        m_ui->exportProgress->setValue(0);
        m_exportBtn->setEnabled(true);
    });

    connect(m_ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, [this]() {
        close();
    });
    m_ui->buttonBox->button(QDialogButtonBox::Close)->setVisible(false);

    connect(&m_assetExporter, &AssetExporter::stateChanged,
            this, &AssetExportDialog::onExportStateChanged);
    connect(&m_assetExporter, &AssetExporter::exportProgressChanged,
            this, &AssetExportDialog::updateExportProgress);

    connect(TaskHub::instance(), &TaskHub::taskAdded, this, &AssetExportDialog::onTaskAdded);

    m_ui->exportProgress->setRange(0,0);
}

AssetExportDialog::~AssetExportDialog()
{
    m_assetExporter.cancel();
}

void AssetExportDialog::onExport()
{
    switchView(true);

    updateExportProgress(0.0);
    TaskHub::clearTasks(Constants::TASK_CATEGORY_ASSET_EXPORT);
    m_exportLogs->clear();

    Utils::FilePath selectedPath = m_ui->exportPath->filePath();
    Utils::FilePath exportPath = m_perComponentExportCheck->isChecked() ?
                (selectedPath.isDir() ? selectedPath : selectedPath.parentDir()) :
                selectedPath;

    m_assetExporter.exportQml(m_filePathModel.files(), exportPath,
                              m_exportAssetsCheck->isChecked(),
                              m_perComponentExportCheck->isChecked());
}

void AssetExportDialog::onExportStateChanged(AssetExporter::ParsingState newState)
{
    switch (newState) {
    case AssetExporter::ParsingState::ExportingDone:
        m_exportBtn->setVisible(false);
        m_ui->buttonBox->button(QDialogButtonBox::Close)->setVisible(true);
        break;
    default:
        break;
    }

    m_exportBtn->setEnabled(newState == AssetExporter::ParsingState::ExportingDone);
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(m_assetExporter.isBusy());
}

void AssetExportDialog::updateExportProgress(double value)
{
    value = std::max(0.0, std::min(1.0, value));
    m_ui->exportProgress->setValue(std::round(value * 1000));
}

void AssetExportDialog::switchView(bool showExportView)
{
    if (showExportView)
        m_ui->stackedWidget->setCurrentWidget(m_exportLogs);
    else
        m_ui->stackedWidget->setCurrentWidget(m_filesView);
}

void AssetExportDialog::onTaskAdded(const ProjectExplorer::Task &task)
{
    Utils::OutputFormat format = Utils::NormalMessageFormat;
    if (task.category == Constants::TASK_CATEGORY_ASSET_EXPORT) {
        switch (task.type) {
        case ProjectExplorer::Task::Error:
            format = Utils::StdErrFormat;
            break;
        case ProjectExplorer::Task::Warning:
            format = Utils::StdOutFormat;
            break;
        default:
            format = Utils::NormalMessageFormat;
        }
        addFormattedMessage(m_outputFormatter, task.description(), format);
    }
}

}
