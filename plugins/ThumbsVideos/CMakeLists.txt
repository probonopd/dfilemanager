find_library(FFMPEGTHUMB_LIBRARY NAMES ffmpegthumbnailer)
find_path(FFMPEGTHUMB_INCLUDE_DIRS NAMES videothumbnailer.h PATH_SUFFIXES libffmpegthumbnailer)
if (FFMPEGTHUMB_LIBRARY AND FFMPEGTHUMB_INCLUDE_DIRS)
    message(STATUS "found ffmpegthumbnailer library ${FFMPEGTHUMB_LIBRARY}, building plugin for thumbnailing videos")
    project(thumbsvideosplugin)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -std=c++11")

    file(GLOB SRCS *.cpp)
    file(GLOB HDRS *.h)

    include_directories(${FFMPEGTHUMB_INCLUDE_DIRS})
    add_library(thumbsvideosplugin SHARED ${SRCS} ${HDRS})

    if (QT5BUILD)
        target_link_libraries(thumbsvideosplugin Qt5::Core Qt5::Gui Qt5::Widgets ${FFMPEGTHUMB_LIBRARY})
    else (QT5BUILD)
        target_link_libraries(thumbsvideosplugin ${QT_LIBRARIES} ${FFMPEGTHUMB_LIBRARY})
    endif (QT5BUILD)

    install(TARGETS thumbsvideosplugin LIBRARY DESTINATION lib/dfm)
endif (FFMPEGTHUMB_LIBRARY AND FFMPEGTHUMB_INCLUDE_DIRS)