include(../../ros_qtc_plugin.pri)
include($$QTCREATOR_SOURCES/src/qtcreatorplugin.pri)

QTW_LIBRARY_PATH = $$(QTERMWIDGET_LIBRARY_PATH)
isEmpty(QTW_LIBRARY_PATH):QTW_LIBRARY_PATH=/usr/local/lib/x86_64-linux-gnu
QTW_INCLUDE_PATH = $$(QTERMWIDGET_INCLUDE_PATH)
isEmpty(QTW_INCLUDE_PATH):QTW_INCLUDE_PATH=/usr/local/include
LIBS += -L$$QTW_LIBRARY_PATH -lqtermwidget5
INCLUDEPATH += $$(QTW_INCLUDE_PATH)
INCLUDEPATH += $$PWD

CONFIG += link_pkgconfig
CONFIG += c++14
PKGCONFIG += yaml-cpp

HEADERS += $$files(*.h) \
    ros_colcon_step.h

SOURCES += $$files(*.cpp) \
    ros_colcon_step.cpp

RESOURCES += ros_project.qrc

FORMS += $$files(*.ui) \
    ros_colcon_step.ui
