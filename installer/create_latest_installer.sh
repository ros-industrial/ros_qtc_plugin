#!/bin/bash

# Absolute path
FILE_PATH=$(readlink -f $0)

# Absolute directory path 
DIR_PATH=`dirname $FILE_PATH`

source "$DIR_PATH/installer_functions.sh"

# Create Installer data for latest versions

INSTALLER_VERSION=1.0.1
INSTALLER_RELEASE_DATE=2018-11-27

QTC_LATEST=1
QTC_MINOR_VERSION=4.5.1
QTC_RELEASE_DATE=2017-12-07
RQTC_MINOR_VERSION=0.2.1
RQTC_RELEASE_DATE=2018-02-06
SortingPriority=200
QMAKE_PATH="/home/larmstrong/Qt5.10.0/5.10.0/gcc_64/bin/qmake" # This must be the same version used for qtcreator.7z and qtcreator_dev.7z

createInstaller

