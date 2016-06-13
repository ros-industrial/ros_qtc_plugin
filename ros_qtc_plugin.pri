QTC_SOURCE = $$(ROS_QTC_SOURCE)/qt-creator
QTC_BUILD = $$(ROS_QTC_SOURCE)/qt-creator-build

## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$QTC_SOURCE

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$QTC_BUILD

## set the QTC_DATA_PATH environment variable to override the setting here
IDE_DATA_PATH = $$QTC_BUILD/share/qtcreator

## set the QTC_INSTALL_DATA_PATH environment variable to override the setting here
INSTALL_DATA_PATH = $$QTC_PREFIX/share/qtcreator
