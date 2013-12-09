 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../dfm/
 HEADERS        += thumbsimages.h
 SOURCES        += thumbsimages.cpp
 TARGET          = $$qtLibraryTarget(thumbsimageplugin)
 
 unix {
    target.path = /usr/lib/dfm
    INSTALLS += target
  }
