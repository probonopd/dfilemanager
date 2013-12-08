 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../
 HEADERS        += thumbsimages.h
 SOURCES        += thumbsimages.cpp
 TARGET          = $$qtLibraryTarget(thumbsimageplugin)
 
 unix {
    target.path = /usr/lib/dfm
    INSTALLS += target
  }
