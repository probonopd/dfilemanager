TEMPLATE = app
HEADERS       = mainwindow.h \
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
    devicemanager.h \
    iojob.h \
    thumbsloader.h \
    preview.h
SOURCES       = main.cpp \
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
    devicemanager.cpp \
    iojob.cpp \
    thumbsloader.cpp \
    preview.cpp \
    config.cpp
RESOURCES     = resources.qrc
DESTDIR= $$PWD

target.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/application
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS application.pro images
sources.path = $$[QT_INSTALL_EXAMPLES]/mainwindows/application
INSTALLS += target sources

QT += gui xml opengl
CONFIG += qt thread debug staticlib
#LIBS += -lglut -lGLU

#libmagic needed for mimetypes and fileinfo
#libsolid needed for devicehandling
unix {
    LIBS += -lmagic -lsolid -lX11
}
