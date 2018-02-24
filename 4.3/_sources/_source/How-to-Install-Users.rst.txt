How to Install (Users)
======================
This wiki explains the procedure for installing the ROS Qt Creator Plug-in.

.. Note:: If you primarily want to use this tool for development of other ROS packages (ie: not to work on the plugin itself), please follow the following instructions.

Installation
------------

.. Important::

   The install method has changed from using the ppa method to a custom installer. This is to enable the ability to provide richer support leveraging existing ros tools which was not possible using the ppa.

Installation Procedure for Ubuntu 16.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Install Prerequsites:

   .. code-block:: bash

      sudo apt install libqtermwidget5-0

#. Next proceed to `Qt Installer Procedure`_


.. Important::

   If previously installed using the ppa please follow the procedure below to remove old version.

   .. code-block:: bash

      sudo apt install ppa-purge
      sudo ppa-purge -o beineri
      sudo ppa-purge levi-armstrong/qt-libraries-xenial
      sudo ppa-purge levi-armstrong/ppa

   .. Warning::

      The ppa-purge removes everything installed from the ppa, so if the ppa is used for other development do not purge.


Installation Procedure for Ubuntu 14.04
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#. Install Prerequsites:

   .. code-block:: bash

      sudo add-apt-repository ppa:levi-armstrong/ppa
      sudo apt install libqtermwidget5-0

#. Next proceed to `Qt Installer Procedure`_


.. Important::

   If previously installed using the ppa please follow the procedure below to remove old version.

   .. code-block:: bash

      sudo apt install ppa-purge
      sudo ppa-purge -o beineri
      sudo ppa-purge levi-armstrong/qt-libraries-xenial

   .. Warning::

      The ppa-purge removes everything installed from the ppa, so if the ppa is used for other development do not purge.

Archived Versions
~~~~~~~~~~~~~~~~~

If for some reason you need a version other than the latest, all installers may be found `here <https://aeswiki.datasys.swri.edu/qtcreator_ros/downloads/installers/>`_.


Qt Installer Procedure
~~~~~~~~~~~~~~~~~~~~~~

#. Download Installer

   #. `Online Installer <https://aeswiki.datasys.swri.edu/qtcreator_ros/downloads/installers/4.3.1/qtcreator-ros-431-online-installer.run>`_ (Recommended)

   #. `Offline Installer <https://aeswiki.datasys.swri.edu/qtcreator_ros/downloads/installers/4.3.1/qtcreator-ros-431-offline-installer.run>`_

   .. Note::

      The Offline Installer is to be used on machines that do not have internet access.

#. Then right click on the installer file, select properties and enable execution under permissions.

#. Next double click the installer and it should open and step throught the installer.


   .. image:: ../_static/installer_page_1.png

|

   .. image:: ../_static/installer_page_2.png

|

   .. image:: ../_static/installer_page_3.png

|

   .. image:: ../_static/installer_page_4.png

|

   .. image:: ../_static/installer_page_5.png

|

   .. image:: ../_static/installer_page_6.png

|

   .. image:: ../_static/installer_page_7.png

|

#. How to get future updates, open Qt Creator and on the menubar under Help select **"Check for Updates"**.

   .. image:: ../_static/get_updates.png


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
