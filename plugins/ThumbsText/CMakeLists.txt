project(thumbstextplugin)

file(GLOB SRCS *.cpp)
file(GLOB HDRS *.h)

add_library(thumbstextplugin SHARED ${SRCS} ${HDRS})

if (QT5BUILD)
    target_link_libraries(thumbstextplugin Qt5::Core Qt5::Gui Qt5::Widgets)
else (QT5BUILD)
    target_link_libraries(thumbstextplugin ${QT_LIBRARIES})
endif (QT5BUILD)

install(TARGETS thumbstextplugin LIBRARY DESTINATION lib/dfm)


