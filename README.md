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

## Installation

[![Get it from the Snap Store](https://snapcraft.io/static/images/badges/en/snap-store-black.svg)](https://snapcraft.io/qtcreator-ros)

You can install Qt Creator with the ROS corresponding plugin via the [snap store](https://snapcraft.io/qtcreator-ros). This will make sure that the Qt Creator version and the ROS plugin version will use matching API and update automatically once a new version has been released. You can install the `qtcreator-ros` snap by searching in the software centre or from the command line:
```bash
sudo snap install qtcreator-ros --classic
```

To install the plugin manually, use the released plugin archives as described in [Plugin Installation](#plugin-installation). To build the plugin from scratch, e.g. for unreleased Qt Creator versions, just follow the build instructions below.

## Dependencies

To build the Qt Creator plugin, you will need Qt Creator, the matching "Plugin Development" package and a recent Qt version. These can be installed either via the official binary installer of via the `setup.py` script. The latter method allows you to adjust the target Qt Creator version via `versions.yaml`.

Additionally, you need:
- OpenGL development libraries
- ninja for building
- yaml-cpp
- qtermwidget
- utf8proc

The can be installed via apt on Ubuntu:
```bash
sudo apt install libgl1-mesa-dev ninja-build libyaml-cpp-dev libqtermwidget5-0-dev libutf8proc-dev
```

To use the `setup.py` script, you will need additional python dependencies:
```bash
pip install pyyaml requests py7zr==0.21
```

## Build

If Qt Creator and the Plugin Development package are not installed in one of the default folders, you have to tell CMake via `CMAKE_PREFIX_PATH` where those can be found.

When using the official Qt binary installer with the default installation path, this is:
```bash
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="~/Qt/Tools/QtCreator/;~/Qt/5.15.2/gcc_64"
```
When using the `setup.py` script, the script will show the installation path (which can be adjusted using `--install_path`) and the compile commands. On `x86_64` Linux with the default path, this is:
```bash
cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="/tmp/qtc_sdk/Tools/QtCreator;/tmp/qtc_sdk/5.15.0/gcc_64"
```

Finally, compile the plugin and create an archive:
```bash
cmake --build build --target package
```
This will create an archive of the format `ROSProjectManager-${version}-Linux-${arch}.zip` inside the build folder (`build` by default). This archive can either be imported by Qt Creator via Help → About Plugins… → Install Plugin… or alternatively extracted directly to `~/Qt/Tools/QtCreator/`.

## Plugin Installation

Download the plugin archive from the [release page](https://github.com/ros-industrial/ros_qtc_plugin/releases/latest) and extract it into the root of a Qt Creator installation. Qt Creator can be installed via the official [online](https://www.qt.io/download-thank-you) and [offline](https://www.qt.io/offline-installers) installer. The Qt Creator root will be `~/Qt/Tools/QtCreator` for the online installer and `~/qtcreator-${version}` for the offline installer. The following script extracts the archive to the default online installer location:
```bash
sudo apt install libarchive-tools # needed for bsdtar
export QTC_ROOT=~/Qt/Tools/QtCreator # online installer
# export QTC_ROOT=~/qtcreator-6.0.0 # offline installer
export PLUGIN_URL=`curl -s https://api.github.com/repos/ros-industrial/ros_qtc_plugin/releases/latest | grep -E 'browser_download_url.*ROSProjectManager-.*-Linux-.*.zip' | cut -d'"' -f 4`
curl -SL $PLUGIN_URL | bsdtar -xzf - -C $QTC_ROOT
```

Note: Qt Creator from the online installer may notify you about available updates and install them when instructed to do so. The plugin API is only compatible with patch updates. A major or minor update will break the plugin API and Qt Creator will then refuse to load the plugin. Make sure that a compatible plugin version is available before updating Qt Creator, since it is not possible to downgrade to an older Qt Creator version using the online installer. The offline installer installs a specific Qt Creator version and does not provide updates.