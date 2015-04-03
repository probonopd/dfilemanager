cmake_minimum_required(VERSION 2.8.11)

project(dfm)

option(QT5BUILD "build using qt5 instead of qt4" OFF)

#this is quite some magic, no need to use qtX_wrap_cpp anymore
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
set(MAIN_DIR ${CMAKE_CURRENT_SOURCE_DIR})
find_package(X11)

#find qt...
if (QT5BUILD)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Gui REQUIRED)
    find_package(Qt5Widgets REQUIRED)
    find_package(Qt5Network REQUIRED)
    find_package(Qt5OpenGL REQUIRED)
    find_package(Qt5Xml REQUIRED)
    find_package(Qt5X11Extras)
    find_package(Qt5DBus)
    include_directories(${Qt5Widgets_INCLUDE_DIRS} ${Qt5X11Extras_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS} ${Qt5OpenGL_INCLUDE_DIRS} ${Qt5Xml_INCLUDE_DIRS})
    add_definitions(${Qt5Widgets_DEFINITIONS})
    
    find_Library(SOLID_LIBRARY NAMES kf5solid)
    if (SOLID_LIBRARY)
        set(SOLID_FOUND ON)
        add_definitions(-DHASSOLID)
        message(STATUS "found kf5solid ${SOLID_LIBRARY}")
    else (SOLID_LIBRARY)
        message(STATUS "kf5solid not found, device management in bookmarksview bad at best")
    endif (SOLID_LIBRARY)
else (QT5BUILD)
    find_package(Qt4 REQUIRED)
    find_package(KDE4)
    include(${QT_USE_FILE})
    include_directories(${QT_INCLUDES})
    add_definitions(${QT_DEFINITIONS} ${QT_QTDBUS_DEFINITIONS})
    
    if (KDE4_FOUND)
	add_definitions(${KDE4_DEFINITIONS} -DHASKDE4)
	include_directories(${KDE4_INCLUDES})
	include(KDE4Defaults)
	include(MacroLibrary)
    endif (KDE4_FOUND)
    
    find_Library(SOLID_LIBRARY NAMES solid)
    if (SOLID_LIBRARY)
        set(SOLID_FOUND ON)
        add_definitions(-DHASSOLID)
        message(STATUS "found solid ${SOLID_LIBRARY}")
     else (SOLID_LIBRARY)
        message(STATUS "solid not found, device management in bookmarksview bad at best")
    endif (SOLID_LIBRARY)
endif (QT5BUILD)

#find magic...
find_library(MAGIC_LIBRARY NAMES magic)
find_file(MAGIC_INCLUDES NAMES magic.h)
find_path(MAGIC_INCLUDE_DIRS NAMES magic.h)
if (MAGIC_LIBRARY AND MAGIC_INCLUDES)
    set(MAGIC_FOUND ON)
    add_definitions(-DHASMAGIC)
    message(STATUS "found magic library: ${MAGIC_LIBRARY} and header: ${MAGIC_INCLUDES}")
    include_directories(${MAGIC_INCLUDE_DIRS})
endif (MAGIC_LIBRARY AND MAGIC_INCLUDES)

#we need to first handle plugins...
add_subdirectory(plugins)
add_subdirectory(dfm)

# get_property(DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach (DIR IN LISTS DIRS)
#     message(STATUS ${DIR})
# endforeach (DIR IN LISTS DIRS)