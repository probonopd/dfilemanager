project(dfm)

file(GLOB DFM_SRCS *.cpp)
file(GLOB DFM_HDRS *.h)

add_executable(dfm ${DFM_SRCS} ${DFM_HDRS})

if (QT5BUILD)
    target_link_libraries(dfm Qt5::Core Qt5::Gui Qt5::Widgets Qt5::DBus Qt5::Network Qt5::OpenGL Qt5::Xml)
else (QT5BUILD)
    target_link_libraries(dfm ${QT_LIBRARIES} ${QT_QTXML_LIBRARY} ${QT_QTOPENGL_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTDBUS_LIBRARY} )
endif (QT5BUILD)

if (X11_FOUND)
    target_link_libraries(dfm ${X11_X11_LIB} ${X11_LIBRARIES})
endif (X11_FOUND)

if (MAGIC_FOUND)
    target_link_libraries(dfm ${MAGIC_LIBRARY})
endif (MAGIC_FOUND)

if (SOLID_FOUND)
    target_link_libraries(dfm ${SOLID_LIBRARY})
endif (SOLID_FOUND)

install(TARGETS dfm DESTINATION bin)

if (UNIX)
    install(FILES dfm.desktop DESTINATION share/applications)
endif (UNIX)
