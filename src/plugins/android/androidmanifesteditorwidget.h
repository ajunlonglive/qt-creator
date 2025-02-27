// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/texteditor.h>

#include <QAbstractListModel>
#include <QGroupBox>
#include <QTabWidget>
#include <QStackedWidget>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QDomDocument;
class QDomElement;
class QComboBox;
class QPushButton;
class QLabel;
class QLineEdit;
class QListView;
class QSpinBox;
class QToolButton;
class QXmlStreamReader;
class QXmlStreamWriter;
QT_END_NAMESPACE

namespace Core { class IEditor; }

namespace Android::Internal {

class AndroidManifestEditor;
class AndroidManifestEditorIconContainerWidget;
class AndroidManifestEditorWidget;
class AndroidServiceWidget;
class SplashScreenContainerWidget;

class PermissionsModel: public QAbstractListModel
{
    Q_OBJECT
public:
    PermissionsModel(QObject *parent = nullptr);
    void setPermissions(const QStringList &permissions);
    const QStringList &permissions();
    QModelIndex addPermission(const QString &permission);
    void removePermission(int index);
    QVariant data(const QModelIndex &index, int role) const override;

protected:
    int rowCount(const QModelIndex &parent) const override;

private:
    QStringList m_permissions;
};

class AndroidManifestTextEditorWidget : public TextEditor::TextEditorWidget
{
public:
    explicit AndroidManifestTextEditorWidget(AndroidManifestEditorWidget *parent);

private:
    Core::IContext *m_context;
};

class AndroidManifestEditorWidget : public QStackedWidget
{
    Q_OBJECT
public:
    enum EditorPage {
        General = 0,
        Source = 1
    };

    explicit AndroidManifestEditorWidget();

    bool isModified() const;

    EditorPage activePage() const;
    bool setActivePage(EditorPage page);

    void preSave();
    void postSave();

    Core::IEditor *editor() const;
    TextEditor::TextEditorWidget *textEditorWidget() const;

    void setDirty(bool dirty = true);

signals:
    void guiChanged();

protected:
    void focusInEvent(QFocusEvent *event) override;

private:
    void defaultPermissionOrFeatureCheckBoxClicked();
    void addPermission();
    void removePermission();
    void updateAddRemovePermissionButtons();
    void setPackageName();
    void updateInfoBar();
    void updateSdkVersions();
    void startParseCheck();
    void delayedParseCheck();
    void initializePage();
    bool syncToWidgets();
    void syncToWidgets(const QDomDocument &doc);
    void syncToEditor();
    void updateAfterFileLoad();

    bool checkDocument(const QDomDocument &doc, QString *errorMessage,
                       int *errorLine, int *errorColumn);

    void updateInfoBar(const QString &errorMessage, int line, int column);
    void hideInfoBar();
    void setInvalidServiceInfo();
    void clearInvalidServiceInfo();

    void parseManifest(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseApplication(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseSplashScreen(QXmlStreamWriter &writer);
    void parseService(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseNewServices(QXmlStreamWriter &writer);
    void parseActivity(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    bool parseMetaData(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseUsesSdk(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    QString parseUsesPermission(QXmlStreamReader &reader,
                                QXmlStreamWriter &writer,
                                const QSet<QString> &permissions);
    QString parseComment(QXmlStreamReader &reader, QXmlStreamWriter &writer);
    void parseUnknownElement(QXmlStreamReader &reader, QXmlStreamWriter &writer, bool ignore=false);

    QGroupBox *createPermissionsGroupBox(QWidget *parent);
    QGroupBox *createPackageFormLayout(QWidget *parent);
    QGroupBox *createApplicationGroupBox(QWidget *parent);
    QGroupBox *createAdvancedGroupBox(QWidget *parent);

    void addServiceMetadata(QXmlStreamWriter &writer);

    bool m_dirty = false; // indicates that we need to call syncToEditor()
    bool m_stayClean = false;
    int m_errorLine;
    int m_errorColumn;
    QString m_currentsplashImageName[3];
    bool m_currentsplashSticky = false;

    QLineEdit *m_packageNameLineEdit;
    QLabel *m_packageNameWarningIcon;
    QLabel *m_packageNameWarning;
    QLineEdit *m_versionCodeLineEdit;
    QLineEdit *m_versionNameLinedit;
    QComboBox *m_androidMinSdkVersion;
    QComboBox *m_androidTargetSdkVersion;

    // Application
    QLineEdit *m_appNameLineEdit;
    QLineEdit *m_activityNameLineEdit;
    QComboBox *m_styleExtractMethod;
    QComboBox *m_screenOrientation;
    AndroidManifestEditorIconContainerWidget *m_iconButtons;
    SplashScreenContainerWidget *m_splashButtons;

    // Permissions
    QCheckBox *m_defaultPermissonsCheckBox;
    QCheckBox *m_defaultFeaturesCheckBox;
    PermissionsModel *m_permissionsModel;
    QListView *m_permissionsListView;
    QPushButton *m_addPermissionButton;
    QPushButton *m_removePermissionButton;
    QComboBox *m_permissionsComboBox;

    // Services
    AndroidServiceWidget *m_services;
    QTimer m_timerParseCheck;
    TextEditor::TextEditorWidget *m_textEditorWidget;
    AndroidManifestEditor *m_editor;
    QString m_androidNdkPlatform;
    QTabWidget *m_advanvedTabWidget = nullptr;
};

} // Android::Internal
