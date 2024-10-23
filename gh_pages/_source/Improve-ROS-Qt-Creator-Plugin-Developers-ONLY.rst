How to Install (Developers)
===========================

.. Note:: If you'd like to contribute to the development of the ROS Qt Creator Plug-in, you are considered a *developer*, please follow the following instructions.

Installation
------------

Installation Dependencies for Ubuntu 20.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    sudo apt update
    sudo apt install libgl1-mesa-dev ninja-build libyaml-cpp-dev libqtermwidget5-0-dev libutf8proc-dev
    sudo apt install python3-pip
    pip install pyyaml requests py7zr

Run ROS Qt Creator setup script
-------------------------------

#. Clone the latest `devel` version:

   .. code-block:: bash

      git clone https://github.com/ros-industrial/ros_qtc_plugin.git -b devel

#. Navigate to `ros_qtc_plugin` and run the setup script to download additional Qt and Qt Creator dependencies:

   .. code-block:: bash

      ./install-sdk.py

.. Note:: The script will download the official binary distributions of Qt and Qt Creator and thus will only support commonly used architectures. If you want to build the plugin on unsupported architectures, you have to build them from source or use a Linux distribution that provides sufficiently new versions of these packages.

.. Note:: The script will download the latest versions that are compatible with the `devel` branch. The versions can be changed in `versions.yaml` to build against an older or newer version.

.. Note:: Alternatively to the setup script, you can use the official binary installer for Qt and Qt Creator. In this case, you have to select the "Plugin Development" package to install the development headers, and "Debug Symbols" if you want your gdb backtrace to contain Qt Creator symbols.

Build plugin
-------------------------------

#. Compile the plugin and create a plugin package

   .. code-block:: bash

      cmake -B build -GNinja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="/tmp/qtc_sdk/Tools/QtCreator;/tmp/qtc_sdk/5.15.0/gcc_64"
      cmake --build build --target package

Testing Plugin
--------------

#. Extract the archive to your Qt Creator installation and execute the command below or launch using the desktop launcher.

   .. code-block:: bash

      qtcreator

#. To verify that the plugin exist, goto File>New File or Project>Projects>Other Project>ROS Workspace. If the ROS Workspace is present then everything built correctly and is ready for development and testing.

Debug issues with Plugin
------------------------

#. The instructions above compile the plugin with debug symbols. You can then debug the plugin by starting Qt Creator with gdb:

   .. code-block:: bash

      gdb --ex=r --args qtcreator

#. After the plugin segfaults, print a backtrace:

   .. code-block:: bash

      (gdb) bt

   and share it on GitHub in a new or active issue.

.. Note:: By default, Qt Creator does not come with debug symbols. The backtrace will only contain symbols of the plugin. Qt Creator debug symbols can be installed either by downloading them from the same source as the setup script, or via the official binary distribution installer (package "Debug Symbols").