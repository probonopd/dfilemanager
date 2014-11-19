 TEMPLATE        = lib
 CONFIG         += plugin
 INCLUDEPATH    += ../../dfm/ /usr/include/poppler/qt4
 HEADERS        += thumbspdf.h
 SOURCES        += thumbspdf.cpp
 TARGET          = $$qtLibraryTarget(thumbspdfplugin)
 LIBS           += -lpoppler-qt4
 
  unix {
    target.path = $$(PREFIX)/lib/dfm
    INSTALLS += target
  }

