cmake_minimum_required(VERSION 2.8.11)

project(dfm)
option(QT5BUILD "build using qt5 instead of qt4" OFF)

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
    
    find_Library(SOLID_LIBRARY NAMES KF5Solid)
    if (SOLID_LIBRARY)
        set(SOLID_FOUND ON)
        add_definitions(-DHASSOLID -DSOLID_VERSION=5)
        message(STATUS "found KF5Solid ${SOLID_LIBRARY}")
        find_path(STD_INCLUDE_PATH NAMES stdio.h)
        find_file(SOLID_DIR NAMES KF5/Solid PATHS ${STD_INCLUDE_PATH})
        if (SOLID_DIR)
            message(STATUS "Found KF5solid include dir: ${SOLID_DIR}")
            include_directories(${SOLID_DIR})
        else (SOLID_DIR)
            message(STATUS "*** No KF5Solid include path found ***")
        endif (SOLID_DIR)
    else (SOLID_LIBRARY)
        message(STATUS "kf5solid not found, device management in bookmarksview bad at best")
    endif (SOLID_LIBRARY)
    
    # uninstall target, stolen from http://www.cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
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
        add_definitions(-DHASSOLID -DSOLID_VERSION=4)
        message(STATUS "found solid ${SOLID_LIBRARY}")
        find_path(STD_INCLUDE_PATH NAMES stdio.h)
        find_file(SOLID_DIR NAMES solid PATHS ${STD_INCLUDE_PATH})
        if (SOLID_DIR)
            message(STATUS "Found solid include dir: ${SOLID_DIR}")
            include_directories(${SOLID_DIR})
        else (SOLID_DIR)
            message(STATUS "*** No include path found solid ***")
        endif (SOLID_DIR)
     else (SOLID_LIBRARY)
        message(STATUS "solid not found, device management in bookmarksview bad at best")
     endif (SOLID_LIBRARY)
endif (QT5BUILD)

if (UNIX)
    add_definitions(-DISUNIX)
elseif (WIN32)
    add_definitions(-DISWINDOWS)
endif (UNIX)

#find x11...
find_package(X11)
if (X11_FOUND)
    add_definitions(-DHASX11)
endif (X11_FOUND)

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

#find sys, sys/statfs.h etc...
find_file(SYS_FILE NAMES sys)
if (SYS_FILE)
    add_definitions(-DHASSYS)
endif (SYS_FILE)

#this is quite some magic, no need to use qtX_wrap_cpp anymore
set(CMAKE_AUTOMOC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#we need to first handle plugins...
add_subdirectory(plugins)
add_subdirectory(dfm)

# get_property(DIRS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
# foreach (DIR IN LISTS DIRS)
#     message(STATUS ${DIR})
# endforeach (DIR IN LISTS DIRS)