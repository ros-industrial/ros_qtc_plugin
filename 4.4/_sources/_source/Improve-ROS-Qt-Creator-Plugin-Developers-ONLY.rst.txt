How to Install (Developers)
===========================

.. Note:: If you'd like to contribute to the development of the ROS Qt Creator Plug-in, you are considered a *developer*, please follow the following instructions.

Installation
------------

Installation Procedure for Ubuntu 14.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    sudo add-apt-repository ppa:ubuntu-toolchain-r/test
    sudo add-apt-repository ppa:levi-armstrong/qt-libraries-trusty
    sudo add-apt-repository ppa:levi-armstrong/ppa
    sudo apt-get update && sudo apt-get install qt57creator-plugin-ros libqtermwidget57-0-dev

Installation Procedure for Ubuntu 16.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo add-apt-repository ppa:levi-armstrong/qt-libraries-xenial
   sudo add-apt-repository ppa:levi-armstrong/ppa
   sudo apt-get update && sudo apt-get install qt57creator-plugin-ros libqtermwidget57-0-dev

Configure system to use the new version of Qt
---------------------------------------------

After installation you need to tell qtchooser where to find this install. Replace the two lines in the file below with the location to the local version shown below. Make sure to change *username* and *version* in the file path to match your system.

File:

.. code-block:: bash

   sudo gedit /usr/lib/x86_64-linux-gnu/qt-default/qtchooser/default.conf

File content:

.. code-block:: bash

   /opt/qt57/bin
   /opt/qt57/lib

Run ROS Qt Creator setup script
-------------------------------

#. Clone your fork of the repository.

   .. code-block:: bash

      git clone -b master https://github.com/<username>/ros_qtc_plugins.git

#. Next in a terminal, navigate into the repository directory and execute the command below.

   .. code-block:: bash

      bash setup.sh -d

.. Note:: Instruction 2 can can be repeated to get the latest updates for Qt Creator source. The developer must manually update there fork to get the latest version of ros_qtc_plugins.

Testing Plugin
--------------

#. Execute the command below or launch using the desktop launcher.

   .. code-block:: bash

      qtcreator

#. To verify that the plugin exist, goto File>New File or Project>Projects>Other Project>ROS Workspace. If the ROS Workspace is present then everything built correctly and is ready for development and testing.

Debug issues with Plugin
------------------------

#. Next in a terminal, navigate to the repository ros_qtc_plugin and execute the command below.

   .. code-block:: bash

      bash setup.sh -di

#. Now launch qtcreator using gdb as shown below and after the plugin segfaults post the trace back in the active/new issue.

   .. code-block:: bash

      gdb <local>/qt-creator-build/bin/qtcreator
      (gdb) run

   After error:

   .. code-block:: bash

      (gdb) bt
