Debugging Catkin Workspace
==========================

Prerequisite
------------

#. Allow ptrace by following these `instructions <Setup-Qt-Creator-for-ROS#31-setup-ubuntu-to-allow-debuggingptrace>`_

Attach to a unstarted process
-----------------------------

#. Next in Qt Creator browse to the file you wish to debug and insert break points.
#. :kbd:`Menu Bar` > :kbd:`Debug` > :kbd:`Start Debugging` > :kbd:`Attach to Unstarted Application...`
#. Browse to the executable then select :kbd:`Start Watching`.
#. Now run your project. :kbd:`Ctrl` + :kbd:`R`
#. Now depending on where the breakpoints were placed in qt, it should be stopped at a break point when it reaches one.

Attach to a running process
---------------------------

#. Next in Qt Creator browse to the file you wish to debug and insert break points.
#. Now run your project. :kbd:`Ctrl` + :kbd:`R`
#. :kbd:`Menu Bar` > :kbd:`Debug` > :kbd:`Start Debugging` > :kbd:`Attach to Running Application...`
#. Now select the Process ID and then click the button :kbd:`Attach to Process`.
#. Now depending on where the breakpoints were placed in qt, it should be stopped at a break point when it reaches one.

.. Note::

   #. Sometime it will be paused at a ros::spin, so check after it has attached and if it is passed and not at a inserted breakpoint click the continue button in the debugger.
   #. Some nodes can execute fast enough to where the debugger is not fully attach to the process before it reaches the first breakpoint, resulting in it never stopping at the breakpoint. To solve this add a `sleep(NumberOfSeconds)` command at the start of the node to allow enough time for the debugger to attach. Usually 3 seconds is enough but varies depending on the size of the node.
