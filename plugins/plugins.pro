TEMPLATE = subdirs
SUBDIRS += ThumbsImages
CONFIG += ordered
unix {
exists("/usr/include/GraphicsMagick/Magick++/Image.h") {
    message("Found GraphicsMagick, building thumbs plugin for pdf files")
    SUBDIRS += ThumbsPDF
}
exists("/usr/include/libffmpegthumbnailer/videothumbnailer.h") {
    message("Found ffmpegthumbnailer, building thumbs plugin for video files")
    SUBDIRS += ThumbsVideos
}
exists("/usr/include/libmoviethumbs/movieclient.h") {
    message("Found MovieThumbs, building thumbs plugin for movies/series files")
    SUBDIRS += ThumbsMovies
}
exists("/usr/include/magic.h") {
    message("Found libMagic, building thumbs plugin for text files")
    SUBDIRS += ThumbsText
}
}
