 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../dfm/ /usr/include/GraphicsMagick
 HEADERS        += thumbspdf.h
 SOURCES        += thumbspdf.cpp
 TARGET          = $$qtLibraryTarget(thumbspdfplugin)
 LIBS           += -lGraphicsMagick++
 
  unix {
    target.path = /usr/lib/dfm
    INSTALLS += target
  }

