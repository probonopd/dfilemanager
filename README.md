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

## Building

Building on Linux with Qt5:

```
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
