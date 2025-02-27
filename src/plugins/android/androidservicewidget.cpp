// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "androidservicewidget.h"
#include "androidservicewidget_p.h"
#include "androidtr.h"

#include <utils/utilsicons.h>

#include <QAbstractTableModel>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>

namespace Android {
namespace Internal {

bool AndroidServiceData::isValid() const
{
    return !m_className.isEmpty()
            && (!m_isRunInExternalProcess || !m_externalProcessName.isEmpty())
            && (!m_isRunInExternalLibrary || !m_externalLibName.isEmpty());
}

void AndroidServiceData::setClassName(const QString &className)
{
    m_className = className;
}

QString AndroidServiceData::className() const
{
    return m_className;
}

void AndroidServiceData::setRunInExternalProcess(bool isRunInExternalProcess)
{
    m_isRunInExternalProcess = isRunInExternalProcess;
    if (!m_isRunInExternalProcess) {
        m_isRunInExternalLibrary = false;
        m_externalProcessName.clear();
        m_externalLibName.clear();
    }
}

bool AndroidServiceData::isRunInExternalProcess() const
{
    return m_isRunInExternalProcess;
}

void AndroidServiceData::setExternalProcessName(const QString &externalProcessName)
{
    if (m_isRunInExternalProcess)
        m_externalProcessName = externalProcessName;
}

QString AndroidServiceData::externalProcessName() const
{
    return m_externalProcessName;
}

void AndroidServiceData::setRunInExternalLibrary(bool isRunInExternalLibrary)
{
    if (m_isRunInExternalProcess)
        m_isRunInExternalLibrary = isRunInExternalLibrary;
    if (!m_isRunInExternalLibrary)
        m_externalLibName.clear();
    else
        m_serviceArguments.clear();
}

bool AndroidServiceData::isRunInExternalLibrary() const
{
    return m_isRunInExternalLibrary;
}

void AndroidServiceData::setExternalLibraryName(const QString &externalLibraryName)
{
    if (m_isRunInExternalLibrary)
        m_externalLibName = externalLibraryName;
}

QString AndroidServiceData::externalLibraryName() const
{
    return m_externalLibName;
}

void AndroidServiceData::setServiceArguments(const QString &serviceArguments)
{
    if (!m_isRunInExternalLibrary)
        m_serviceArguments = serviceArguments;
}

QString AndroidServiceData::serviceArguments() const
{
    return m_serviceArguments;
}

void AndroidServiceData::setNewService(bool isNewService)
{
    m_isNewService = isNewService;
}

bool AndroidServiceData::isNewService() const
{
    return m_isNewService;
}

void AndroidServiceWidget::AndroidServiceModel::setServices(const QList<AndroidServiceData> &androidServices)
{
    beginResetModel();
    m_services = androidServices;
    endResetModel();
}

const QList<AndroidServiceData> &AndroidServiceWidget::AndroidServiceModel::services()
{
    return m_services;
}

void AndroidServiceWidget::AndroidServiceModel::addService()
{
    int rowIndex = m_services.size();
    beginInsertRows(QModelIndex(), rowIndex, rowIndex);
    AndroidServiceData service;
    service.setNewService(true);
    m_services.push_back(service);
    endInsertRows();
    emit invalidDataChanged();
}

void AndroidServiceWidget::AndroidServiceModel::removeService(int row)
{
    beginRemoveRows(QModelIndex(), row, row);
    m_services.removeAt(row);
    endRemoveRows();
}

void AndroidServiceWidget::AndroidServiceModel::servicesSaved()
{
    for (auto && x : m_services)
        x.setNewService(false);
}

int AndroidServiceWidget::AndroidServiceModel::rowCount(const QModelIndex &/*parent*/) const
{
    return m_services.count();
}

int AndroidServiceWidget::AndroidServiceModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 6;
}

Qt::ItemFlags AndroidServiceWidget::AndroidServiceModel::flags(const QModelIndex &index) const
{
    if (index.column() == 0)
        return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
    else if (index.column() == 1)
        return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
    else if (index.column() == 2 && index.row() <= m_services.count()) {
        if (m_services[index.row()].isRunInExternalProcess())
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
        return Qt::ItemIsSelectable;
    } else if (index.column() == 3 && index.row() <= m_services.count()) {
        if (m_services[index.row()].isRunInExternalProcess())
            return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
        return Qt::ItemIsUserCheckable | Qt::ItemIsSelectable;
    } else if (index.column() == 4 && index.row() <= m_services.count()) {
        if (m_services[index.row()].isRunInExternalLibrary())
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
        return Qt::ItemIsSelectable;
    } else if (index.column() == 5 && index.row() <= m_services.count()) {
        if (!m_services[index.row()].isRunInExternalLibrary())
            return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable;
        return Qt::ItemIsSelectable;
    }
    return Qt::ItemIsSelectable;
}

QVariant AndroidServiceWidget::AndroidServiceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::ToolTipRole && orientation == Qt::Horizontal) {
        if (section == 0)
            return Tr::tr("The name of the class implementing the service.");
        else if (section == 1)
            return Tr::tr("Checked if the service is run in an external process.");
        else if (section == 2)
            return Tr::tr("The name of the external process.\n"
                          "Prefix with : if the process is private, use a lowercase name if the process is global.");
        else if (section == 3)
            return Tr::tr("Checked if the service is in a separate dynamic library.");
        else if (section == 4)
            return Tr::tr("The name of the separate dynamic library.");
        else if (section == 5)
            return Tr::tr("The arguments for telling the app to run the service instead of the main activity.");
    } else if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        if (section == 0)
            return Tr::tr("Service class name.");
        else if (section == 1)
            return Tr::tr("Run in external process.");
        else if (section == 2)
            return Tr::tr("Process name.");
        else if (section == 3)
            return Tr::tr("Run in external library.");
        else if (section == 4)
            return Tr::tr("Library name.");
        else if (section == 5)
            return Tr::tr("Service arguments.");
    }
    return {};
}

QVariant AndroidServiceWidget::AndroidServiceModel::data(const QModelIndex &index, int role) const
{
    if (!(index.row() >= 0 && index.row() < m_services.count()))
        return {};
    if (role == Qt::CheckStateRole) {
        if (index.column() == 3)
            return m_services[index.row()].isRunInExternalLibrary() ? Qt::Checked : Qt::Unchecked;
        else if (index.column() == 1 && index.row() <= m_services.count())
            return m_services[index.row()].isRunInExternalProcess() ? Qt::Checked : Qt::Unchecked;
        return QVariant();
    } else if (role == Qt::DisplayRole) {
        if (index.column() == 0)
            return m_services[index.row()].className();
        else if (index.column() == 1)
            return Tr::tr("Run in external process.");
        else if (index.column() == 2)
            return m_services[index.row()].externalProcessName();
        else if (index.column() == 3)
            return Tr::tr("Run in external library.");
        else if (index.column() == 4)
            return m_services[index.row()].externalLibraryName();
        else if (index.column() == 5)
            return m_services[index.row()].serviceArguments();
    } else if (role == Qt::ToolTipRole) {
        if (index.column() == 0 && m_services[index.row()].className().isEmpty())
            return Tr::tr("The class name must be set.");
        else if (index.column() == 2 && m_services[index.row()].isRunInExternalProcess())
            return Tr::tr("The process name must be set for a service run in an external process.");
        else if (index.column() == 4 && m_services[index.row()].isRunInExternalLibrary())
            return Tr::tr("The library name must be set for a service run in an external library.");
    } else if (role == Qt::EditRole) {
        if (index.column() == 0)
            return m_services[index.row()].className();
        else if (index.column() == 2)
            return m_services[index.row()].externalProcessName();
        else if (index.column() == 4)
            return m_services[index.row()].externalLibraryName();
        else if (index.column() == 5)
            return m_services[index.row()].serviceArguments();
    } else if (role == Qt::DecorationRole) {
        if (index.column() == 0) {
            if (m_services[index.row()].className().isEmpty())
                return Utils::Icons::WARNING.icon();
        } else if (index.column() == 2) {
            if (m_services[index.row()].isRunInExternalProcess()
                    && m_services[index.row()].externalProcessName().isEmpty())
                return Utils::Icons::WARNING.icon();
        } else if (index.column() == 4) {
            if (m_services[index.row()].isRunInExternalLibrary()
                    && m_services[index.row()].externalLibraryName().isEmpty())
                return Utils::Icons::WARNING.icon();
        }
    }
    return {};
}

bool AndroidServiceWidget::AndroidServiceModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!(index.row() >= 0 && index.row() < m_services.count()))
        return {};
    if (role == Qt::CheckStateRole) {
        if (index.column() == 1)
            m_services[index.row()].setRunInExternalProcess((value == Qt::Checked) ? true : false);
        else if (index.column() == 3)
            m_services[index.row()].setRunInExternalLibrary((value == Qt::Checked) ? true : false);
        emit dataChanged(createIndex(index.row(), 0), createIndex(index.row(), 5));
        if (m_services[index.row()].isValid())
            emit validDataChanged();
        else
            emit invalidDataChanged();
    } else if (role == Qt::EditRole) {
        if (index.column() == 0) {
            QString className = value.toString();
            if (!className.isEmpty() && className[0] != QChar('.'))
                className.push_front(QChar('.'));
            m_services[index.row()].setClassName(className);
            m_services[index.row()].setNewService(true);
        } else if (index.column() == 2) {
            m_services[index.row()].setExternalProcessName(value.toString());
        } else if (index.column() == 4) {
            m_services[index.row()].setExternalLibraryName(value.toString());
        } else if (index.column() == 5) {
            m_services[index.row()].setServiceArguments(value.toString());
        }
        emit dataChanged(index, index);
        if (m_services[index.row()].isValid())
            emit validDataChanged();
        else
            emit invalidDataChanged();
    }
    return true;
}

AndroidServiceWidget::AndroidServiceWidget(QWidget *parent) : QWidget(parent),
  m_model(new AndroidServiceModel), m_tableView(new QTableView(this))
{
    m_tableView->setModel(m_model.data());
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    QSizePolicy sizePolicy;
    sizePolicy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_tableView->setSizePolicy(sizePolicy);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    auto layout = new QHBoxLayout(this);
    layout->addWidget(m_tableView, 1);
    auto buttonLayout = new QGridLayout();
    auto addButton = new QPushButton(this);
    addButton->setText(Tr::tr("Add"));
    buttonLayout->addWidget(addButton, 0, 0);
    m_removeButton = new QPushButton(this);
    m_removeButton->setText(Tr::tr("Remove"));
    m_removeButton->setEnabled(false);
    buttonLayout->addWidget(m_removeButton, 1, 0);
    layout->addLayout(buttonLayout);
    layout->setAlignment(buttonLayout, Qt::AlignTop);
    connect(addButton, &QAbstractButton::clicked,
            this, &AndroidServiceWidget::addService);
    connect(m_removeButton, &QAbstractButton::clicked,
            this, &AndroidServiceWidget::removeService);
    connect(m_tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            [this](const QItemSelection &selected, const QItemSelection &/*deselected*/) {
        if (!selected.isEmpty())
            m_removeButton->setEnabled(true);
    });
    connect(m_model.data(), &AndroidServiceWidget::AndroidServiceModel::validDataChanged,
            [this] { emit servicesModified(); });
    connect(m_model.data(), &AndroidServiceWidget::AndroidServiceModel::invalidDataChanged,
            [this] { emit servicesInvalid(); });
}

AndroidServiceWidget::~AndroidServiceWidget()
{

}

void AndroidServiceWidget::setServices(const QList<AndroidServiceData> &androidServices)
{
    m_removeButton->setEnabled(false);
    m_model->setServices(androidServices);
}

const QList<AndroidServiceData> &AndroidServiceWidget::services()
{
    return m_model->services();
}

void AndroidServiceWidget::servicesSaved()
{
    m_model->servicesSaved();
}

void AndroidServiceWidget::addService()
{
    m_model->addService();
}

void AndroidServiceWidget::removeService()
{
    auto selections = m_tableView->selectionModel()->selectedRows();
    for (const auto &x : selections) {
        m_model->removeService(x.row());
        m_removeButton->setEnabled(false);
        emit servicesModified();
        break;
    }
}

} // namespace Internal
} // namespace Android
