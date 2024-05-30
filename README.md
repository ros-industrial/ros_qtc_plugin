# ros_qtc_plugin
This repository contains simple to install and use ROS-related plugins for the Qt Creator IDE. See the repository [wiki](https://ros-qtc-plugin.readthedocs.io) for installation and additional information.

## Overview
The ROS Qt Creator Plug-in is developed specifically for ROS to increase a developers' efficiency by simplifying tasks and creating a centralized location for ROS tools. Since it is built on top of the Qt Creator platform, users have access to all of its existing features like: syntax highlighting, code indexing, editors (C++ , Python, etc.), code completion, version control (Git, Subversion, etc.), debuggers (GDB, CDB, LLDB, etc.), and much more.

The ROS Qt Creator Plug-in provides the following capabilities:
 * Import/Create Catkin Workspaces
 * Create Catkin Packages
 * Custom Build Configuration
   * CatkinMake
   * CatkinTools
 * Custom Run Configurations
   * roslaunch
   * rosrun
   * Attach to Node
 * Automatic sourcing of the workspace
 * Integrated Tabbed Terminal
 * Templates
   * Industrial Robot Support Package
   * Basic Launch File
   * Basic URDF File
   * Basic Node File

## Installation (Binary)

### Snap with Qt Creator and Plugin

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/qtcreator-ros)

You can install Qt Creator with the ROS corresponding plugin via the [snap store](https://snapcraft.io/qtcreator-ros). This will make sure that the Qt Creator version and the ROS plugin version will use matching API and update automatically once a new version has been released. You can install the `qtcreator-ros` snap by searching in the software centre or from the command line:
```bash
sudo snap install qtcreator-ros --classic
```

### Manual Qt Creator and Plugin Installation

Qt Creator can be installed via the official [online](https://www.qt.io/download-qt-installer-oss) and [offline](https://www.qt.io/offline-installers) installer.

Download the plugin archive from the [release page](https://github.com/ros-industrial/ros_qtc_plugin/releases/latest) and extract it into the root of a Qt Creator installation. The Qt Creator root will be `~/Qt/Tools/QtCreator` for the online installer and `~/qtcreator-${version}` for the offline installer.

 The following script extracts the archive to the default online installer location:
```bash
sudo apt install libarchive-tools # needed for bsdtar
curl -SL $(curl -s https://api.github.com/repos/ros-industrial/ros_qtc_plugin/releases/latest | grep -E 'browser_download_url.*ROSProjectManager-.*-Linux-.*.zip' | cut -d'"' -f 4) | bsdtar -xzf - -C ~/Qt/Tools/QtCreator
```

Note: Qt Creator from the online installer may notify you about available updates and install them when instructed to do so. The plugin API is only compatible with patch-level updates. A major or minor update will break the plugin API and Qt Creator will then refuse to load the plugin. Make sure that a compatible plugin version is available before updating Qt Creator as it is not possible to downgrade to an older Qt Creator version using the online installer. The offline installer installs a specific Qt Creator version and does not provide updates.

## Build (Source)

### Dependencies

To build the Qt Creator plugin, you will need Qt Creator, the matching "Plugin Development" package and a recent Qt version. These can be installed either via the official binary installer or via the [`setup.py`](setup.py) script. The latter method allows you to adjust the target Qt Creator version via [`versions.yaml`](versions.yaml).

Additionally, you need:
- OpenGL development libraries
- ninja for building
- yaml-cpp
- utf8proc

The dependencies can be installed via apt on Ubuntu:
```bash
sudo apt install libgl1-mesa-dev ninja-build libyaml-cpp-dev libutf8proc-dev
```

The `setup.py` script needs additional Python dependencies:
```bash
pip install pyyaml requests py7zr==0.21
```

Install Qt Creator, its development files and Qt to a folder of your choice:
```sh
./setup.py --install_path ~/Downloads/
```
The script will print the CMake commands for building the plugin and create an archive.

### Build the Plugin Archive

If Qt Creator and the Plugin Development package are not installed in one of the default folders, you have to tell CMake via `CMAKE_PREFIX_PATH` where those can be found. With the `setup.py` command above, this could be (adjust `CMAKE_PREFIX_PATH` and the Qt version if necessary):
```sh
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="~/Downloads/qtc-sdk/Tools/QtCreator;~/Downloads/qtc-sdk/6.6.0/gcc_64"
cmake --build build --target package
```
This will create the plugin archive `ROSProjectManager-${version}-Linux-${arch}.zip` inside the build folder (`build` by default). This archive has to be extracted to the Qt Creator root path (e.g. `~/Qt/Tools/QtCreator/` for the online installer version).

### Development & Debugging

To develop on the plugin and test changes iteratively, build the plugin in `Debug` mode and skip creating the plugin archive:
```sh
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="~/Downloads/qtc-sdk/Tools/QtCreator;~/Downloads/qtc-sdk/6.6.0/gcc_64"
cmake --build build
```
This creates a shared plugin library at `build/lib/qtcreator/plugins/libROSProjectManager.so`.

To test the changes to the plugin, launch Qt Creator and point it to the location of the compiled plugin:
```sh
~/Downloads/qtc-sdk/Tools/QtCreator/bin/qtcreator -pluginpath build/lib/qtcreator/plugins/
```
To debug the plugin with `gdb`, prefix the above command with `gdb --ex=r --args`:
```sh
gdb --ex=r --args ~/Downloads/qtc-sdk/Tools/QtCreator/bin/qtcreator -pluginpath build/lib/qtcreator/plugins/
```

To further debug the inner workings of Qt Creator, you have to install the Qt Creator "Debug Symbols". If you are using the online installer, those are available via the "Qt Maintenance Tool".
