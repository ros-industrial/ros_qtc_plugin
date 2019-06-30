****************
Perf and friends
****************
There is a tool called `perf`_ that can collect data of the system. This can be targeted to collect data of your program and then parse that dump of data to be visualized. The parser is called `perfparser`_ from the Qt people and, along with it, they have also developed a plugin to visualize data directly in the IDE as shown in the image below. However this data can be visualized with other programs as the KDAB people show with the project `Hotspot`_.

	.. image:: ../_static/flame_graph.png

System setup
============
1. Install perf. In Ubuntu like systems:

.. code-block:: shell

	sudo apt install linux-tools-common linux-tools-generic

2. Allow to run perf system wide by running. If is the first time, reboot:

.. code-block:: shell

    echo -e "kernel.perf_event_paranoid=-1\nkernel.kptr_restrict=0" | sudo tee /etc/sysctl.d/10-perf.conf

If by running `cat /proc/sys/kernel/kptr_restrict` the result is 0, the permissions are set right.

QtCreator Setup
===============
1. Install Qt Creator with the ROS plugin by `dowloading the latest version`_.
2. Setup the workspace in QtCreator
3. Compile (Shortcut with Ctrl+B)
4. Add the custom executable. With catkin, all executables are outputed under *<workspace>/devel/lib/<my_package>*. We will create a configuration in QtCreator so it can use it. We will rename the custom executable to make it easier to find and use afterwards.

	.. image:: ../_static/custom_executable.gif

Performance analysis
====================
1. Go to the debug view.
2. Select the custom executable created before from the bottom left menu.
3. Change the tool to **Performance Analyzer**
4. Start the debug session with the play button. The program should run.
5. Start recording data by pressing the grey circle.
6. Stop recording data by pressing the red circle again. Red means that is recording.
7. Stop the node (if you want to, this step assumes free will).
8. Et voila! You can visualize your data, a very useful view is the *Flame graph*.

In the gif below it can be seen how the *veryComplicatedStuff* function is taking 85% of the execution of the process.

	.. image:: ../_static/performance_analysis.gif

Bonus: Running tests from QtCreator
===================================
Qt Creator also supports Google Tests natively and you can, not only run an specific test from the IDE but also debug it!. To do so:

1. Compile the tests with `catkin run_tests <my_package>`. This will generate a binary with the tests.
2. Add the custom executable. As before, create a custom executable but, this time, the target should be the previously generated binary.
3. Go the Tests view and make use of it.
4. When running the tests, it will ask about which *Custom executable* to use. Select the one created right now.

	.. image:: ../_static/tests.gif

.. _perf: https://en.wikipedia.org/wiki/Perf_(Linux)
.. _perfparser: https://code.qt.io/cgit/qt-creator/perfparser.git/tree/
.. _Hotspot: https://github.com/KDAB/hotspot
.. _dowloading the latest version: //https://github.com/ros-industrial/ros_qtc_plugin/releases