## set the QTC_SOURCE environment variable to override the setting here
## Following variables are exporeted in setup.sh.
## If running qmake manually, please export QTC_SOURCE, QTC_BUILD
## else QTCREATOR_SOURCES will default to $$PWD/../qt-creator and
## IDE_BUILD_TREE will default $$PWD/../qt-creator-build

message("== ros_qtc_plugin.pri Enviroment")
QTCREATOR_SOURCES = $$(QTC_SOURCE)
isEmpty(QTCREATOR_SOURCES):QTCREATOR_SOURCES=$$PWD/../qt-creator
message("== QTCREATOR_SOURCES: $$QTCREATOR_SOURCES")

## set the QTC_BUILD environment variable to override the setting here
IDE_BUILD_TREE = $$(QTC_BUILD)
isEmpty(IDE_BUILD_TREE):IDE_BUILD_TREE=$$PWD/../qt-creator-build
message("== IDE_BUILD_TREE: $$IDE_BUILD_TREE")

## set the QTC_DATA_PATH environment variable to override the setting here
IDE_DATA_PATH = $$IDE_BUILD_TREE/share/qtcreator
message("== IDE_DATA_PATH: $$IDE_DATA_PATH")

## set the QTC_INSTALL_DATA_PATH environment variable to override the setting here
INSTALL_DATA_PATH = $$QTC_PREFIX/share/qtcreator
message("== INSTALL_DATA_PATH: $$INSTALL_DATA_PATH")
