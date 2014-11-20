 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../dfm/
 HEADERS        += thumbstext.h
 SOURCES        += thumbstext.cpp
 TARGET          = $$qtLibraryTarget(thumbstextplugin)

  unix {
    target.path = /usr/lib/dfm
    INSTALLS += target
  }
