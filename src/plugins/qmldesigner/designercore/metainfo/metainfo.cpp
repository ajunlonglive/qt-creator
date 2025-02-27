// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "metainfo.h"

#include "modelnode.h"
#include "metainforeader.h"
#include "iwidgetplugin.h"

#include <invalidmetainfoexception.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/filepath.h>

#include "pluginmanager/widgetpluginmanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QMutex>

enum {
    debug = false
};

namespace QmlDesigner {
namespace Internal {


static QString globalMetaInfoPath()
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/globalMetaInfo";
#endif
    return Core::ICore::resourcePath("qmldesigner/globalMetaInfo").toString();
}

Utils::FilePaths allGlobalMetaInfoFiles()
{
    static Utils::FilePaths paths;

    if (!paths.isEmpty())
        return paths;

    QDirIterator it(globalMetaInfoPath(), { "*.metainfo" }, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext())
        paths.append(Utils::FilePath::fromString(it.next()));

    return paths;
}

class MetaInfoPrivate
{
    Q_DISABLE_COPY(MetaInfoPrivate)
public:
    MetaInfoPrivate(MetaInfo *q);
    void clear();

    void initialize();

    void parseItemLibraryDescriptions();

    QScopedPointer<ItemLibraryInfo> m_itemLibraryInfo;

    MetaInfo *m_q;
    bool m_isInitialized;
};

MetaInfoPrivate::MetaInfoPrivate(MetaInfo *q)
    : m_itemLibraryInfo(new ItemLibraryInfo())
    , m_q(q)
    , m_isInitialized(false)
{
    if (!m_q->isGlobal())
        m_itemLibraryInfo->setBaseInfo(MetaInfo::global().itemLibraryInfo());
}

void MetaInfoPrivate::clear()
{
    m_itemLibraryInfo->clearEntries();
    m_isInitialized = false;
}

namespace {
bool enableParseItemLibraryDescriptions = true;
}

void MetaInfoPrivate::initialize()
{
    if (enableParseItemLibraryDescriptions)
        parseItemLibraryDescriptions();
    m_isInitialized = true;
}

void MetaInfoPrivate::parseItemLibraryDescriptions()
{
    Internal::WidgetPluginManager pluginManager;
    for (const QString &pluginDir : std::as_const(m_q->s_pluginDirs))
        pluginManager.addPath(pluginDir);
    const QList<IWidgetPlugin *> widgetPluginList = pluginManager.instances();
    for (IWidgetPlugin *plugin : widgetPluginList) {
        Internal::MetaInfoReader reader(*m_q);
        try {
            reader.readMetaInfoFile(plugin->metaInfo());
        } catch (const InvalidMetaInfoException &e) {
            qWarning() << e.description();
            const QString errorMessage = plugin->metaInfo() + QLatin1Char('\n') + QLatin1Char('\n')
                                         + reader.errors().join(QLatin1Char('\n'));
            Core::AsynchronousMessageBox::warning(
                QCoreApplication::translate("QmlDesigner::Internal::MetaInfoPrivate",
                                            "Invalid meta info"),
                errorMessage);
        }
    }

    const Utils::FilePaths allMetaInfoFiles = allGlobalMetaInfoFiles();
    for (const Utils::FilePath &path : allMetaInfoFiles) {
        Internal::MetaInfoReader reader(*m_q);
        try {
            reader.readMetaInfoFile(path.toString());
        } catch (const InvalidMetaInfoException &e) {
            qWarning() << e.description();
            const QString errorMessage = path.toString() + QLatin1Char('\n') + QLatin1Char('\n')
                                         + reader.errors().join(QLatin1Char('\n'));
            Core::AsynchronousMessageBox::warning(
                QCoreApplication::translate("QmlDesigner::Internal::MetaInfoPrivate",
                                            "Invalid meta info"),
                errorMessage);
        }
    }
}

} // namespace Internal

void MetaInfo::disableParseItemLibraryDescriptionsUgly()
{
    Internal::enableParseItemLibraryDescriptions = false;
}

using QmlDesigner::Internal::MetaInfoPrivate;

MetaInfo MetaInfo::s_global;
QMutex s_lock;
QStringList MetaInfo::s_pluginDirs;


/*!
\class QmlDesigner::MetaInfo
\ingroup CoreModel
\brief The MetaInfo class provides meta information about QML types and
properties.
*/

/*!
  Constructs a copy of \a metaInfo.
  */
MetaInfo::MetaInfo(const MetaInfo &metaInfo) = default;

/*!
  Creates a meta information object with just the QML types registered statically.
  You almost always want to use Model::metaInfo() instead.

  You almost certainly want to access the meta information for the model.

  \sa Model::metaInfo()
  */
MetaInfo::MetaInfo() :
        m_p(new MetaInfoPrivate(this))
{}

MetaInfo::~MetaInfo() = default;

/*!
  Assigns \a other to this meta information and returns a reference to this
  meta information.
  */
MetaInfo& MetaInfo::operator=(const MetaInfo &other) = default;

ItemLibraryInfo *MetaInfo::itemLibraryInfo() const
{
    return m_p->m_itemLibraryInfo.data();
}

/*!
  Accesses the global meta information object.
  You almost always want to use Model::metaInfo() instead.

  Internally, all meta information objects share this \e global object
  where static QML type information is stored.
  */
MetaInfo MetaInfo::global()
{
    QMutexLocker locker(&s_lock);

    if (!s_global.m_p->m_isInitialized) {
        s_global.m_p = QSharedPointer<MetaInfoPrivate>(new MetaInfoPrivate(&s_global));
        s_global.m_p->initialize();
    }
    return s_global;
}

/*!
  Clears the global meta information object.

  This function should be called once on application shutdown to free static data structures.
  */
void MetaInfo::clearGlobal()
{
    if (s_global.m_p->m_isInitialized)
        s_global.m_p->clear();
}

void MetaInfo::setPluginPaths(const QStringList &paths)
{
    s_pluginDirs = paths;
    global();
    clearGlobal();
}

bool MetaInfo::isGlobal() const
{
    return (this->m_p == s_global.m_p);
}

bool operator==(const MetaInfo &first, const MetaInfo &second)
{
    return first.m_p == second.m_p;
}

bool operator!=(const MetaInfo &first, const MetaInfo &second)
{
    return !(first == second);
}

} //namespace QmlDesigner
