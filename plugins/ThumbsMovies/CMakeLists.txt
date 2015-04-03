set(MOVIETHUMBS_PATH /usr/include/libmoviethumbs /usr/local/include/libmoviethumbs)
find_library(MOVIETHUMBS_LIBRARY NAMES moviethumbs)
find_file(MOVIETHUMBS_INCLUDES NAMES movieclient.h PATHS ${MOVIETHUMBS_PATH})
find_path(MOVIETHUMBS_INCLUDE_DIRS NAMES movieclient.h PATHS ${MOVIETHUMBS_PATH})
if (MOVIETHUMBS_LIBRARY AND MOVIETHUMBS_INCLUDES)
    project(thumbsmoviesplugin)
    message(STATUS "found moviethumbs library: ${MOVIETHUMBS_LIBRARY} and header: ${MOVIETHUMBS_INCLUDES}, building moviethumbs thumbnailing plugin...")
    include_directories(${MOVIETHUMBS_INCLUDE_DIRS})
    
    file(GLOB SRCS *.cpp)
    file(GLOB HDRS *.h)

    add_library(thumbsmoviesplugin SHARED ${SRCS} ${HDRS})

    if (QT5BUILD)
        target_link_libraries(thumbsmoviesplugin Qt5::Core Qt5::Gui Qt5::Widgets ${MOVIETHUMBS_LIBRARY})
    else (QT5BUILD)
        target_link_libraries(thumbsmoviesplugin ${QT_LIBRARIES} ${MOVIETHUMBS_LIBRARY})
    endif (QT5BUILD)

    install(TARGETS thumbsmoviesplugin LIBRARY DESTINATION lib/dfm)
endif (MOVIETHUMBS_LIBRARY AND MOVIETHUMBS_INCLUDES)


