# ros_qtc_plugin
This contains ROS related plugins for Qt Creator (4.4). See the repository [wiki](http://ros-industrial.github.io/ros_qtc_plugin/) for additional information.

### Important (Install method has changed)!
The install method has changed from using the ppa method to a custom installer. This is to enable the ability to provide richer support leveraging existing ros tools which was not possible using the ppa. Please follow the procedure outlined in [How to Install (Users)](https://ros-industrial.github.io/ros_qtc_plugin/_source/How-to-Install-Users.html) to get the latest version moving forward.

### Overview
The ROS Qt Creator Plug-in is developed specifically for ROS to increase a developers' efficiency by simplifying tasks and creating a centralized location for ROS tools. Since it is built on top of the Qt Creator platform, users have access to all of its existing features like: syntax highlighting, editors (C++ , Python, etc.), code completion, version control (Git, Subversion, etc.), debuggers (GDB, CDB, LLDB, etc.), and much more.

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

