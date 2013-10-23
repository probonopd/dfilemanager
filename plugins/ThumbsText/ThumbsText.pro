 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../
 HEADERS        += thumbstext.h
 SOURCES        += thumbstext.cpp
 TARGET          = $$qtLibraryTarget(thumbstextplugin)
 DESTDIR         = ./plugins
 LIBS           += -lmagic
