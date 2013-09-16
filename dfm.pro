TEMPLATE = app
HEADERS = mainwindow.h \
    iconview.h \
    placesview.h \
    deletedialog.h \
    icondialog.h \
    filesystemmodel.h \
    flowview.h \
    detailsview.h \
    columnsview.h \
    viewcontainer.h \
    tabbar.h \
    searchbox.h \
    settingsdialog.h \
    pathnavigator.h \
    propertiesdialog.h \
    config.h \
    viewanimator.h \
    iconprovider.h \
    dockwidget.h \
    titlewidget.h \
    infowidget.h \
    Proxystyle/proxystyle.h \
    application.h \
    operations.h \
    recentfoldersview.h \
    iojob.h \
    thumbsloader.h \
    preview.h \
    columnswidget.h
SOURCES = main.cpp \
    mainwindow.cpp \
    iconview.cpp \
    placesview.cpp \
    settings.cpp \
    actions.cpp \
    deletedialog.cpp \
    icondialog.cpp \
    filesystemmodel.cpp \
    flowview.cpp \
    detailsview.cpp \
    columnsview.cpp \
    viewcontainer.cpp \
    tabbar.cpp \
    searchbox.cpp \
    settingsdialog.cpp \
    pathnavigator.cpp \
    propertiesdialog.cpp \
    viewanimator.cpp \
    iconprovider.cpp \
    dockwidget.cpp \
    Proxystyle/proxystyle.cpp \
    application.cpp \
    operations.cpp \
    infowidget.cpp \
    recentfoldersview.cpp \
    iojob.cpp \
    thumbsloader.cpp \
    preview.cpp \
    config.cpp \
    columnswidget.cpp
#RESOURCES = resources.qrc
TARGET = dfm

target.path += $$[QT_INSTALL_BINS]
sources.files = $$SOURCES $$HEADERS #$$RESOURCES #$$FORMS
INSTALLS += target

QT += gui xml opengl
CONFIG += qt thread debug staticlib

#libmagic needed for mimetypes and fileinfo
#libsolid needed for devicehandling
unix {
    LIBS += -lmagic -lsolid -lX11
    INSTALLS += desktop postinstall

    desktop.path = $$[QT_INSTALL_PREFIX]/share/applications
    desktop.files += dfm.desktop

    postinstall.path = $$[QT_INSTALL_BINS]
    postinstall.extra = update-desktop-database
}
