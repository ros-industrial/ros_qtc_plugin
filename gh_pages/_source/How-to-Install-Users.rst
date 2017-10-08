How to Install (Users)
======================
This wiki explains the procedure for installing the ROS Qt Creator Plug-in.

.. Note:: If you primarily want to use this tool for development of other ROS packages (ie: not to work on the plugin itself), please follow the following instructions.

Installation
------------

Installation Procedure for Ubuntu 14.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo add-apt-repository ppa:levi-armstrong/qt-libraries-trusty
   sudo add-apt-repository ppa:levi-armstrong/ppa
   sudo apt-get update && sudo apt-get install qt57creator
   sudo apt-get install qt57creator-plugin-ros

May need to remove old PPA:
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo add-apt-repository --remove ppa:beineri/opt-qt57-trusty

*If you receive an error, then manually remove it.*

.. code-block:: bash

   sudo rm /etc/apt/sources.list.d/beineri-opt-qt57-trusty-trusty.list
   sudo rm /etc/apt/sources.list.d/beineri-opt-qt571-trusty-trusty.list

Installation Procedure for Ubuntu 16.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

   sudo add-apt-repository ppa:levi-armstrong/qt-libraries-xenial
   sudo add-apt-repository ppa:levi-armstrong/ppa
   sudo apt update && sudo apt install qt57creator
   sudo apt install qt57creator-plugin-ros

May need to remove old PPA:
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code-block:: bash

   sudo add-apt-repository --remove ppa:beineri/opt-qt57-xenial
   sudo add-apt-repository --remove ppa:beineri/opt-qt571-xenial

*If you receive an error, then manually remove it.*

.. code-block:: bash

   sudo rm /etc/apt/sources.list.d/beineri-opt-qt57-xenial-xenial.list
   sudo rm /etc/apt/sources.list.d/beineri-opt-qt571-xenial-xenial.list

Installation Issues and Conflicts
---------------------------------

* No known issues

Testing Plugin.
---------------

#. Start Qt Creator

   * Option 1: Launch using the desktop icon.
   * Option 2: Launch from terminal. Before you can launch from the terminal perform one of the action below.

     * Add "source /opt/qt57/bin/qt57-env.sh" to the users .bashrc file.
     * Create a symbolic link file in you /usr/local/bin called qtcreator which points to /opt/qt57/bin/qtcreator-wrapper

#. To verify that the plugin exist, goto File>New File or Project>Projects>Other Project>ROS Workspace. If the ROS Workspace is present then everything built correctly and is ready for development and testing.
