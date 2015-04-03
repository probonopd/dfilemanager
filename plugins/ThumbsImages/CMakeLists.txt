project(thumbsimageplugin)

file(GLOB SRCS *.cpp)
file(GLOB HDRS *.h)

add_library(thumbsimageplugin SHARED ${SRCS} ${HDRS})

if (QT5BUILD)
    target_link_libraries(thumbsimageplugin Qt5::Core Qt5::Gui Qt5::Widgets)
else (QT5BUILD)
    target_link_libraries(thumbsimageplugin ${QT_LIBRARIES})
endif (QT5BUILD)

install(TARGETS thumbsimageplugin LIBRARY DESTINATION lib/dfm)

