set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

set(YAMLCPP_USE_STATIC_LIBS TRUE)

find_package(QtCreator COMPONENTS Core REQUIRED)
find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(YamlCpp REQUIRED)
find_package(qtermwidget5 REQUIRED)

set(SRC
  "remove_directory_dialog.cpp"
  "ros_build_configuration.cpp"
  "ros_build_system.cpp"
  "ros_catkin_make_step.cpp"
  "ros_catkin_test_results_step.cpp"
  "ros_catkin_tools_step.cpp"
  "ros_colcon_step.cpp"
  "ros_generic_run_step.cpp"
  "ros_package_wizard.cpp"
  "ros_packagexml_parser.cpp"
  "ros_project.cpp"
  "ros_project_nodes.cpp"
  "ros_project_plugin.cpp"
  "ros_project_wizard.cpp"
  "ros_rosattach_step.cpp"
  "ros_roslaunch_step.cpp"
  "ros_rosrun_step.cpp"
  "ros_rostest_step.cpp"
  "ros_run_configuration.cpp"
  "ros_run_step.cpp"
  "ros_run_steps_page.cpp"
  "ros_settings_page.cpp"
  "ros_terminal_pane.cpp"
  "ros_utils.cpp"
)

# add the resource file
list(APPEND SRC "ros_project.qrc")

add_qtc_plugin(${PROJECT_NAME}
  PLUGIN_DEPENDS
    QtCreator::CMakeProjectManager
    QtCreator::Core
    QtCreator::CppTools
    QtCreator::Debugger
    QtCreator::ProjectExplorer
    QtCreator::QtSupport
    QtCreator::TextEditor
  DEPENDS
    QtCreator::Utils
    Qt${QT_VERSION_MAJOR}::Widgets
    ${YAMLCPP_STATIC}
    qtermwidget5
  SOURCES
    ${SRC}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/share/
        DESTINATION ${IDE_DATA_PATH}
)
