 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../ /usr/include/GraphicsMagick
 HEADERS        += thumbspdf.h
 SOURCES        += thumbspdf.cpp
 TARGET          = $$qtLibraryTarget(thumbspdfplugin)
 DESTDIR         = ./plugins
 LIBS           += -lGraphicsMagick++
