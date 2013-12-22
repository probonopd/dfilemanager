TEMPLATE = subdirs
SUBDIRS += ThumbsImages
CONFIG += ordered
unix {
SUBDIRS += ThumbsPDF ThumbsText ThumbsVideos ThumbsMovies
}
