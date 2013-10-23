 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../
 HEADERS        += thumbsvideos.h
 SOURCES        += thumbsvideos.cpp
 TARGET          = $$qtLibraryTarget(thumbsvideosplugin)
 DESTDIR         = ./plugins
 LIBS           += -lffmpegthumbnailer
