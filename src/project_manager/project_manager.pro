include(../../ros_qtc_plugins.pri)
include(project_manager.pri)
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

QTW_LIBRARY_PATH = $$(QTERMWIDGET_LIBRARY_PATH)
isEmpty(QTW_LIBRARY_PATH):QTW_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu
QTW_INCLUDE_PATH = $$(QTERMWIDGET_INCLUDE_PATH)
isEmpty(QTW_INCLUDE_PATH):QTW_INCLUDE_PATH=/usr/local/include
LIBS += -L$$QTW_LIBRARY_PATH -lqtermwidget5
INCLUDEPATH += $$(QTW_INCLUDE_PATH)

HEADERS = \
    ros_project_wizard.h \
    ros_project_plugin.h \
    ros_project_manager.h \
    ros_project.h \
    ros_project_constants.h \
    ros_make_step.h \
    ros_build_configuration.h \
    ros_project_nodes.h \
    ros_utils.h \
    ros_run_configuration.h \
    ros_run_steps_page.h \
    ros_run_step.h \
    ros_run_steps.h \
    ros_terminal_pane.h \
    ros_package_wizard.h \
    remove_directory_dialog.h \
    ros_workspace_watcher.h

SOURCES = \
    ros_project_wizard.cpp \
    ros_project_plugin.cpp \
    ros_project_manager.cpp \
    ros_project.cpp \
    ros_make_step.cpp \
    ros_build_configuration.cpp \
    ros_project_nodes.cpp \
    ros_utils.cpp \
    ros_run_configuration.cpp \
    ros_run_steps_page.cpp \
    ros_run_step.cpp \
    ros_run_steps.cpp \
    ros_terminal_pane.cpp \
    ros_package_wizard.cpp \
    remove_directory_dialog.cpp \
    ros_workspace_watcher.cpp

RESOURCES += ros_project.qrc
FORMS += ros_make_step.ui \
    ros_build_configuration.ui \
    ros_import_wizard_page.ui \
    ros_run_configuration.ui \
    ros_generic_configuration.ui \
    ros_package_wizard_details_page.ui \
    remove_directory_dialog.ui


