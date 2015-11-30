QTC_SOURCE = /home/larmstrong/ros_qt_plugin/qt-creator
QTC_BUILD = /home/larmstrong/ros_qt_plugin/qt-creator-build

## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$QTC_SOURCE

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$QTC_BUILD

include(ros_project_manager_dependencies.pri)
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)
#include($$QTCREATOR_SOURCES/src/plugins/coreplugin/coreplugin_dependencies.pri)

HEADERS = \
    ros_project_wizard.h \
    ros_project_plugin.h \
    ros_project_manager.h \
    ros_project.h \
    ros_project_constants.h \
    ros_make_step.h \
    ros_build_configuration.h \
    ros_project_nodes.h \
    ros_project_files_editor.h

SOURCES = \
    ros_project_wizard.cpp \
    ros_project_plugin.cpp \
    ros_project_manager.cpp \
    ros_project.cpp \
    ros_make_step.cpp \
    ros_build_configuration.cpp \
    ros_project_nodes.cpp \
    ros_project_files_editor.cpp

RESOURCES += ros_project.qrc
FORMS += ros_make_step.ui
