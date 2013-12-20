 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../dfm/ /usr/include/libmoviethumbs/
 HEADERS        += thumbsmovies.h
 SOURCES        += thumbsmovies.cpp
 TARGET          = $$qtLibraryTarget(thumbsmoviesplugin)
 LIBS           += -lmoviethumbs

  unix {
    target.path = /usr/lib/dfm
    INSTALLS += target
  }
