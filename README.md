# DFileManager

File manager written in Qt/C++, it does use one library from kdelibs, the solid lib for easy device handling.

Mirrored from https://sourceforge.net/p/dfilemanager/.

## Features

* Categorized bookmarks
* Simple searching and filtering
* Customizable Thumbnails for filetypes like multi media stuff
* Can remember some view properties for each folder (unix only)
* Four Different views
* Breadcrumbs navigationbar
* Tabs

There is also [StyleProject](https://sourceforge.net/projects/styleproject/), a style that can go along with it for a Mac-like experience.

## Building

Building with Qt5 on Ubuntu:

```
sudo apt-get -y install qt5-default qt5-qmake libqt5x11extra5-dev libffmpegthumnailer-dev libpoppler-qt5-dev libkf5solid-dev libmagic-dev
cd dfilemanager
mkdir build
cd build
cmake ../ -DCMAKE_INSTALL_PREFIX=/usr -DQT5BUILD=ON
make
sudo make install
```

It can also be built with Qt4.

## Homepage

http://dfilemanager.sourceforge.net/
