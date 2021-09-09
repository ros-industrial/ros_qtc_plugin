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

### Tips
  * Users may create custom templates.

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
pip install pyyaml requests py7zr
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
