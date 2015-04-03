file(GLOB DIRS Thumbs*)

foreach (DIR IN LISTS DIRS)
    message(STATUS "found plugin dir ${DIR}")
    add_subdirectory(${DIR})
endforeach (DIR IN LISTS DIRS)