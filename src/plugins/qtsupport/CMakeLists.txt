add_qtc_plugin(QtSupport
  DEPENDS Qt5::Xml
  PUBLIC_DEPENDS ProParser
  PLUGIN_DEPENDS Core ProjectExplorer ResourceEditor
  SOURCES
    baseqtversion.cpp baseqtversion.h
    codegenerator.cpp codegenerator.h
    codegensettings.cpp codegensettings.h
    codegensettingspage.cpp codegensettingspage.h
    exampleslistmodel.cpp exampleslistmodel.h
    gettingstartedwelcomepage.cpp gettingstartedwelcomepage.h
    profilereader.cpp profilereader.h
    qscxmlcgenerator.cpp qscxmlcgenerator.h
    qtbuildaspects.cpp qtbuildaspects.h
    qtconfigwidget.cpp qtconfigwidget.h
    qtcppkitinfo.cpp qtcppkitinfo.h
    qtkitinformation.cpp qtkitinformation.h
    qtoptionspage.cpp qtoptionspage.h
    qtoutputformatter.cpp qtoutputformatter.h
    qtparser.cpp qtparser.h
    qtprojectimporter.cpp qtprojectimporter.h
    qtsupport.qrc
    qtsupport_global.h
    qtsupporttr.h
    qtsupportconstants.h
    qtsupportplugin.cpp qtsupportplugin.h
    qttestparser.cpp qttestparser.h
    qtversionfactory.h
    qtversionmanager.cpp qtversionmanager.h
    qtversions.cpp qtversions.h
    translationwizardpage.cpp translationwizardpage.h
    uicgenerator.cpp uicgenerator.h
)
