project(thumbsimageplugin)

file(GLOB SRCS *.cpp)
file(GLOB HDRS *.h)

add_library(thumbsimageplugin SHARED ${SRCS} ${HDRS})

if (QT5BUILD)
    target_link_libraries(thumbsimageplugin Qt5::Core Qt5::Gui Qt5::Widgets)
else (QT5BUILD)
    target_link_libraries(thumbsimageplugin ${QT_LIBRARIES})
endif (QT5BUILD)

find_library(EXIV_LIBRARY NAMES exiv2)
find_path(EXIV_INCLUDE_DIRS NAMES exiv2/exiv2.hpp)

if (EXIV_LIBRARY AND EXIV_INCLUDE_DIRS)
    add_definitions(-DHASEXIV)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
    include_directories(${EXIV_INCLUDE_DIRS})
    target_link_libraries(thumbsimageplugin ${EXIV_LIBRARY})
    message(STATUS "Found exiv library at ${EXIV_LIBRARY} and exiv include dirs at ${EXIV_INCLUDE_DIRS}")
else (EXIV_LIBRARY AND EXIV_INCLUDE_DIRS)
    message(STATUS "*** No exiv found ***")
endif (EXIV_LIBRARY AND EXIV_INCLUDE_DIRS)

install(TARGETS thumbsimageplugin LIBRARY DESTINATION lib/dfm)

