 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../
 HEADERS        += thumbsvideos.h
 SOURCES        += thumbsvideos.cpp
 TARGET          = $$qtLibraryTarget(thumbsvideosplugin)
 LIBS           += -lffmpegthumbnailer

  unix {
    target.path = /usr/lib/dfm
    INSTALLS += target
  }
