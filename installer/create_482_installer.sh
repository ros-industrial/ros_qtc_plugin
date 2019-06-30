#!/bin/bash

# Absolute path
FILE_PATH=$(readlink -f $0)

# Absolute directory path 
DIR_PATH=`dirname $FILE_PATH`

source "$DIR_PATH/installer_functions.sh"

# Create Installer data for latest versions
INSTALLER_VERSION=1.0.7 # This must be incremented for every new release
INSTALLER_RELEASE_DATE=2019-05-20

QTC_LATEST=1
QTC_REPO="https://github.com/qt-creator/qt-creator.git"
QTC_VERSION=4.8.2.0
QTC_BRANCH=v4.8.2
QTC_RELEASE_DATE=2019-03-01
RQTC_VERSION=0.3.5.0
RQTC_RELEASE_DATE=2019-05-20
QTERMWIDGET_BRANCH="debian/xenial"
SortingPriority=200

CLANG_FILENAME=libclang-release_60-linux-Rhel7.2-gcc5.3-x86_64-clazy.7z # Name must be located here https://download.qt.io/development_releases/prebuilt/libclang/
YAML_CPP_TAG=yaml-cpp-0.6.2
QT_PATH=/home/Qt5.12.3/5.12.3/gcc_64 # This must be the same version used for qtcreator.7z and qtcreator_dev.7z
QIFW_PATH=/home/QtIFW-3.0.2/bin/binarycreator

createInstaller

