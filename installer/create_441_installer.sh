#!/bin/bash

# Absolute path
FILE_PATH=$(readlink -f $0)

# Absolute directory path 
DIR_PATH=`dirname $FILE_PATH`

source "$DIR_PATH/installer_functions.sh"

# Create Installer data for version 4.4.1
INSTALLER_VERSION=1.0.1
INSTALLER_RELEASE_DATE=2018-11-27

QTC_LATEST=0
QTC_MINOR_VERSION=4.4.1
QTC_RELEASE_DATE=2017-10-04
RQTC_MINOR_VERSION=0.1.8
RQTC_RELEASE_DATE=2018-11-22
SortingPriority=51
QMAKE_PATH="/home/larmstrong/Qt5.9.2/5.9.2/gcc_64/bin/qmake" # This must be the same version used for qtcreator.7z and qtcreator_dev.7z

createInstaller
