How to Install (Users)
======================
This wiki explains the procedure for installing the ROS Qt Creator Plug-in.

.. Note:: If you primarily want to use this tool for development of other ROS packages (ie: not to work on the plugin itself), please follow the following instructions.

Installation
------------
Installation Procedure for Ubuntu 16.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo add-apt-repository ppa:levi-armstrong/qt-libraries-xenial
   sudo add-apt-repository ppa:levi-armstrong/ppa
   sudo apt update && sudo apt install qt59creator
   sudo apt install qt57creator-plugin-ros

.. Note:: 

   Occasionally the Qt dependency is upgraded and when this occures the following procedure is recommended.
 
   .. code-block:: bash
    
      sudo apt remove --purge '^qtXX.*'
      sudo apt update && sudo apt install qtYYcreator
      sudo apt install qt57creator-plugin-ros

   *Remember to replace XX with the current version installed and replace YY with the new version.*



Installation Procedure for Ubuntu 14.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo add-apt-repository ppa:ubuntu-toolchain-r/test
   sudo add-apt-repository ppa:levi-armstrong/qt-libraries-trusty
   sudo add-apt-repository ppa:levi-armstrong/ppa
   sudo apt-get update && sudo apt-get install qt59creator
   sudo apt-get install qt57creator-plugin-ros

.. Note:: 

   Occasionally the Qt dependency is upgraded and when this occures the following procedure is recommended.
 
   .. code-block:: bash
    
      sudo apt remove --purge '^qtXX.*'
      sudo apt update && sudo apt install qtYYcreator
      sudo apt install qt57creator-plugin-ros

   *Remember to replace XX with the current version installed and replace YY with the new version.*

Installation Issues and Conflicts
---------------------------------

* No known issues

Testing Plugin.
---------------

#. Start Qt Creator

   * Option 1: Launch using the desktop icon.
   * Option 2: Launch from terminal. 

     .. code-block:: bash
        
        qtcreator-ros

#. To verify that the plugin exist, goto File>New File or Project>Projects>Other Project>ROS Workspace. If the ROS Workspace is present then everything built correctly and is ready for development and testing.
