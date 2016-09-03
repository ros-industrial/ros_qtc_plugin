## set the QTC_SOURCE environment variable to override the setting here
QTCREATOR_SOURCES = $$QTC_SOURCE
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=$$PWD/../qt-creator

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$QTC_BUILD
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=$$PWD/../qt-creator-build

## set the QTC_DATA_PATH environment variable to override the setting here
IDE_DATA_PATH = $$IDE_BUILD_TREE/share/qtcreator

## set the QTC_INSTALL_DATA_PATH environment variable to override the setting here
INSTALL_DATA_PATH = $$QTC_PREFIX/share/qtcreator
