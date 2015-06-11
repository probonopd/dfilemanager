file(GLOB DIRS Thumbs*)

foreach (DIR IN LISTS DIRS)
    add_subdirectory(${DIR})
endforeach (DIR IN LISTS DIRS)
