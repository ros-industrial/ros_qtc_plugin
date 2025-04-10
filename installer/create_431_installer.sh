#!/bin/bash

# Absolute path
FILE_PATH=$(readlink -f $0)

# Absolute directory path 
DIR_PATH=`dirname $FILE_PATH`

source "$DIR_PATH/installer_functions.sh"

# Create Installer data for version 4.3.1
INSTALLER_VERSION=1.0.1
INSTALLER_RELEASE_DATE=2018-11-27

QTC_LATEST=0
QTC_VERSION=4.3.1.1
QTC_RELEASE_DATE=2017-06-29
RQTC_VERSION=0.1.6.1
RQTC_RELEASE_DATE=2018-11-22
SortingPriority=200

CLANG_FILENAME=libclang-release_39-linux-Rhel6.6-gcc4.9-x86_64.7z # Name must be located here https://download.qt.io/development_releases/prebuilt/libclang/
YAML_CPP_TAG=yaml-cpp-0.6.2
QT_PATH=/home/Qt5.10.0/5.10.0/gcc_64 # This must be the same version used for qtcreator.7z and qtcreator_dev.7z
QIFW_PATH=/home/QtIFW-3.0.2/bin/binarycreator

createInstaller



