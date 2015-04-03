set(POPPLER_PATH /usr/include/poppler/qt4 /usr/local/include/poppler/qt4)
find_library(POPPLER_LIBRARY NAMES poppler-qt4)
find_file(POPPLER_INCLUDES NAMES poppler-qt4.h PATHS ${POPPLER_PATH})
find_path(POPPLER_INCLUDE_DIRS NAMES poppler-qt4.h PATHS ${POPPLER_PATH})
if (NOT QT5BUILD)
if (POPPLER_LIBRARY AND POPPLER_INCLUDES)
    project(thumbspdfplugin)
    message(STATUS "found poppler library: ${POPPLER_LIBRARY} and header: ${POPPLER_INCLUDES}, building poppler thumbnailing plugin...")
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
endif (POPPLER_LIBRARY AND POPPLER_INCLUDES)
endif (NOT QT5BUILD)
