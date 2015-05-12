
if (QT5BUILD)
    find_library(POPPLER_LIBRARY NAMES poppler-qt5)
    find_path(POPPLER_INCLUDE_DIRS NAMES poppler-qt5.h PATH_SUFFIXES poppler/qt5)
        if (POPPLER_LIBRARY AND POPPLER_INCLUDE_DIRS)
        add_definitions(-DPOPPLER_VERSION=5)
    endif (POPPLER_LIBRARY AND POPPLER_INCLUDE_DIRS)
else (QT5BUILD)
    find_library(POPPLER_LIBRARY NAMES ppoppler-qt4)
    find_path(POPPLER_INCLUDE_DIRS NAMES poppler-qt4.h PATH_SUFFIXES poppler/qt4)
    if (POPPLER_LIBRARY AND POPPLER_INCLUDE_DIRS)
        add_definitions(-DPOPPLER_VERSION=4)
    endif (POPPLER_LIBRARY AND POPPLER_INCLUDE_DIRS)
endif (QT5BUILD)

if (POPPLER_LIBRARY AND POPPLER_INCLUDE_DIRS)
    project(thumbspdfplugin)
    message(STATUS "found poppler library: ${POPPLER_LIBRARY} and includes: ${POPPLER_INCLUDE_DIRS}, building poppler thumbnailing plugin...")
    include_directories(${POPPLER_INCLUDE_DIRS})
    
    file(GLOB SRCS *.cpp)
    file(GLOB HDRS *.h)

    add_library(thumbspdfplugin SHARED ${SRCS} ${HDRS})

    if (QT5BUILD)
        target_link_libraries(thumbspdfplugin Qt5::Core Qt5::Gui Qt5::Widgets ${POPPLER_LIBRARY})
    else (QT5BUILD)
        target_link_libraries(thumbspdfplugin ${QT_LIBRARIES} ${POPPLER_LIBRARY})
    endif (QT5BUILD)

    install(TARGETS thumbspdfplugin LIBRARY DESTINATION lib/dfm)
endif (POPPLER_LIBRARY AND POPPLER_INCLUDE_DIRS)

