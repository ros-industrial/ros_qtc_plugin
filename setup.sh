#!/bin/bash

SECONDS=0
LSB=/usr/bin/lsb_release

RED=
GREEN=
YELLOW=
NC=

ncolors=$(tput colors)
if test -n "$ncolors" && test $ncolors -ge 8; then
    RED='\033[0;31m'
    GREEN='\033[1;32m'
    YELLOW='\033[1;93m'
    NC='\033[0m' # No Color
fi

# Verbosity level
VERBOSE=0
CPU_CORES=`grep -c ^processor /proc/cpuinfo`

# User option (-u | -d | -ud | -di)
RUN_TYPE=""

# Absolute path to this setup.sh script: /home/user/project/ros_qtc_plugin/setup.sh
SCRIPT_FILE_PATH=$(readlink -f $0)
# Absolute path to this setup.sh script: /home/user/project/ros_qtc_plugin
QTR_DIR_PATH=`dirname $SCRIPT_FILE_PATH`
# Script folder name:                     ros_qtc_plugin
QTR_DIR_NAME=`basename ${QTR_DIR_PATH}`

#Save initial path
INIT_CWD=`pwd`

# Space seletated missing packages to be installed
PKG_MISSING=""
QMAKE_PATH="/opt/qt57/bin/qmake"

# ROS qtc plubin default branch
QTR_BRANCH="master"
# QT Creator default branch
QTC_BRANCH="4.2"

# By default clean all on rebuild
#Qt Creator
QTC_SKIP_CLEAN=0
#ROS Plugin
QTR_SKIP_CLEAN=0

QTC_PATH=""

QTC_BUILD=""
QTC_SOURCE=""

QTR_BUILD=""
QTR_SOURCE=""

# Log file of all actions
LOG_FILE="${QTR_DIR_PATH}/setup.log"
# Get Git hash for logging purposes
GIT_HASH=`git log --pretty=format:'%h' -n 1`

GIT_QUIET="--quiet"

function deleteLog {
    rm -rf "$LOG_FILE"
}

function logV {
    [[ $VERBOSE -eq 1 ]] && echo -e "$1"
    echo -e "$1" >> "$LOG_FILE"
}

function logC {
    [[ $VERBOSE -eq 1 ]] && echo -e "${YELLOW}$1${NC}"
    echo -e "$1" >> "$LOG_FILE"
}

function logP {
    echo -e "${GREEN}$1${NC}"   # Print color to screen
    echo -e "$1" >> "$LOG_FILE" # No color to log file
}

function logE {
    echo -e "${RED}$1${NC}"   # Print color to screen
    echo -e "$1" >> "$LOG_FILE" # No color to log file
}

function logDuration {
    local duration=$SECONDS
    logP "==  $(($duration / 3600)) hr, $((($duration / 60) % 60)) min, $(($duration % 60)) sec elapsed"
}

function logErrorAndQuit {
    logDuration
    logE "$1"
    # Go back to original location
    cd $INIT_CWD &> /dev/null
    exit -1
}

function testForError {
    retval=$?

    if [ $retval -ne 0 ]; then
        logErrorAndQuit "==  ERROR Operation canceled. Check setup.log"
    fi
}

function printNoTabs() {
    logV "==  ${1//$'\t'/ }"
}

function printUsage {
    logP "Usage: bash setup.sh argument"
    logP "Arguments:"
    logP "  -u            : run setup for users"
    logP "  -d            : run setup for developers"
    logP "  -ui           : run setup for users with debug info"
    logP "  -di           : run setup for developers with debug info"
    logP "  -qtb tag      : build qt creator with branch/tag"
#   logP "  -qtp path: qtcreator path. If provided -qtb is unused"
    logP "  -qtm path     : qmake path"
    logP "  -no-qtc-clean : skip qt creator \"make clean\""
    logP "  -no-qtr-clean : skip ros qt plugin \"make clean\""
    logP "  -no-clean     : skip \"make clean\" for both projects"
    logP "  -v            : verbose mode"
    logP "Defaults"
    logP "  QTCreator : $QTC_BRANCH"
    logP "  ROS Plugin: $QTR_BRANCH"
    logP "  QMake     : $QMAKE_PATH"
    logP "  Verbose   : $VERBOSE"
    # Go back to original location
    cd $INIT_CWD &> /dev/null
    exit 85
}

function logOSInfo() {
    logC "=="
        logC "== Operating system: $(uname)"
    local osText

        if [ -x $LSB ]; then
        osText=`$LSB -i`; printNoTabs "${osText}"
        osText=`$LSB -r`; printNoTabs "${osText}"
        osText=`$LSB -c`; printNoTabs "${osText}"
        osText=`uname -m`; printNoTabs "Hardware: ${osText}"
        osText=`uname -v`; printNoTabs "Kernel: ${osText}"
    fi
}

function loopEach() {
  for path in $1; do
      logV "==  $path"
  done
}

function logEnvInfo() {
    local envText

    logC "=="
    logC "== QT Enviroment Variables"
    envText=`env | grep QT`; loopEach "${envText}"

    logC "=="
    logC "== ROS Enviroment Variables"
    envText=`env | grep ROS`; loopEach "${envText}"

    logC "=="
    logC "== CMAKE Enviroment Variables"
    envText=`env | grep -i cmake`; loopEach "${envText}"
}

function cloneQtCreator {
    logP "==  Cloning QT Creator $QTC_BRANCH. Stand by..."

    local CMD="git clone $GIT_QUIET --depth 1 --single-branch --branch $QTC_BRANCH https://github.com/qtproject/qt-creator.git $QTC_SOURCE"

    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"

    $CMD &>> "$LOG_FILE"

    testForError
    logDuration
}

function pullQtCreator {
    QPATH=$(basename "$PWD")
    logP "==  Fetching into $QPATH"
    local CMD="git fetch $GIT_QUIET"

    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"

    $CMD &>> "$LOG_FILE"

    testForError
    logDuration

    logP "==  Pulling into $QPATH"
    CMD="git pull $GIT_QUIET"

    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"

    $CMD &>> "$LOG_FILE"

    testForError
    logDuration
}

function cloneROSQtPlugin {
    cd $BASE_PATH
    logP "==  Cloning ROS QTC Plugin($QTR_BRANCH). Stand by..."
    local CMD="git clone -depth 1 --single-branch --branch $QTR_BRANCH https://github.com/ros-industrial/ros_qtc_plugin.git"

    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"

    $CMD &>> "$LOG_FILE"

    testForError
    logDuration
}

function pullROSQtPlugin {
    logP "== Entering $BASE_PATH/$QTR_DIR_NAME"
    cd $BASE_PATH/$QTR_DIR_NAME

    QPATH=$(basename "$PWD")
    logP "==  Fetching into $QPATH"

    git $GIT_QUIET fetch &>> "$LOG_FILE"

    testForError

    logP "==  Pulling into $QPATH"
    git $GIT_QUIET pull &>> "$LOG_FILE"

    testForError
    logDuration
}

function build {
    if [ "$2" == "clean" ]; then
        logP "==  Cleaning project $1"

        if [ -e "Makefile" ]; then
            make clean &>> "$LOG_FILE"

            testForError
            logDuration
        fi
    else
        logP "==  Building project $1"

        if [ -e "Makefile" ]; then
            make -j$CPU_CORES &>> "$LOG_FILE"

            testForError
            logDuration
        fi
    fi
}

function logGitHash {
    logP "== ROS Qt Plugin Git($GIT_HASH)"
}

function setParameters {
    if (([ "$1" == "-u" ] || [ "$1" == "-ui" ]) &&
         [ $(basename "$PWD") != 'ros_qtc_plugin' ]); then
        BASE_PATH=$PWD/ros_qtc_plugin
        mkdir -p $BASE_PATH
    else
        BASE_PATH=$(dirname "$PWD")
    fi

    export QTC_SOURCE=$BASE_PATH/qt-creator-${QTC_BRANCH}
    export QTC_BUILD=$BASE_PATH/qt-creator-${QTC_BRANCH}-build

    export QTR_SOURCE=$BASE_PATH/${QTR_DIR_NAME}
    export QTR_BUILD=$BASE_PATH/${QTR_DIR_NAME}-${QTC_BRANCH}-build

    DESKTOP_FILE=$HOME/.local/share/applications/Qt-Creator-Ros-${QTC_BRANCH}.desktop
}

function checkParameters {
    if [ -z "$QMAKE_PATH" ]; then
        QMAKE_PATH="/opt/qt57/bin/qmake"
    fi

    if [ ! -e "$QMAKE_PATH" ]; then
        logErrorAndQuit "== $QMAKE_PATH is mising !!!"
    fi

    if [ ! -x "$QMAKE_PATH" ]; then
        logErrorAndQuit "== $QMAKE_PATH is not executable !!!"
    fi
}

function logRQTEnvironment {
    if [[ $VERBOSE -eq 1 ]]; then
        GIT_QUIET=""
        logC "=="
        logC "== RQT Enviroment Variables"
        logV "== QMAKE_PATH      : $QMAKE_PATH"
        logV "== LOG_FILE        : $LOG_FILE"
        logV "== BASE_PATH       : $BASE_PATH"
        logV "== QTC_SKIP_CLEAN  : $QTC_SKIP_CLEAN"
        logV "==  QTC_BRANCH     : $QTC_BRANCH"
        logV "==  QTC_PATH       : $QTC_PATH"
        logV "==  QTC_SOURCE     : $QTC_SOURCE"
        logV "==  QTC_BUILD      : $QTC_BUILD"
        logV "==  DESKTOP_FILE   : $DESKTOP_FILE"
        logV "== QTR_DIR_PATH    : $QTR_DIR_PATH"
        logV "==  QTR_BRANCH     : $QTR_BRANCH"
        logV "==  QTR_SOURCE     : $QTR_SOURCE"
        logV "==  QTR_BUILD      : $QTR_BUILD"
        logV "==  QTR_SKIP_CLEAN : $QTR_SKIP_CLEAN"
    fi
}

function checkPkgDependency {
    # Install build dependencies
    logP "=="
    logP "== Checking build dependencies"

    PKG_BLD_ESSENTIAL=$(dpkg-query -W --showformat='${Status}\n'\
                        build-essential 2>/dev/null | grep -c "ok installed")
    PKG_MESA_DEV=$(dpkg-query -W --showformat='${Status}\n'\
                        libgl1-mesa-dev 2>/dev/null | grep -c "ok installed")
    PKG_QT_TERM=$(dpkg-query -W --showformat='${Status}\n'\
                        libqtermwidget57-0-dev 2>/dev/null | grep -c "ok installed")
    PKG_CATKIN_TOOLS=$(dpkg-query -W --showformat='${Status}\n'\
                        python-catkin-tools 2>/dev/null | grep -c "ok installed")
    PKG_YAML_CPP_DEV=$(dpkg-query -W --showformat='${Status}\n'\
                        libyaml-cpp-dev 2>/dev/null | grep -c "ok installed")

    if [[ $PKG_BLD_ESSENTIAL -eq 0 ]]; then
        logE "==  Missing        : build-essential"
        PKG_MISSING="$PKG_MISSING build-essential"
    else
        logP "==  Installed      : build-essential"
    fi

    if [[ $PKG_MESA_DEV -eq 0 ]]; then
        logE "==  Missing        : libgl1-mesa-dev"
        PKG_MISSING="$PKG_MISSING libgl1-mesa-dev"
    else
        logP "==  Installed      : libgl1-mesa-dev"
    fi

    if [[ $PKG_QT_TERM -eq 0 ]]; then
        logE "==  Missing        : libqtermwidget57-0-dev"
        PKG_MISSING="$PKG_MISSING libqtermwidget57-0-dev"
    else
        logP "==  Installed      : libqtermwidget57-0-dev"
    fi

    if [[ $PKG_CATKIN_TOOLS -eq 0 ]]; then
        logE "==  Missing        : python-catkin-tools"
        PKG_MISSING="$PKG_MISSING python-catkin-tools"
    else
        logP "==  Installed      : python-catkin-tools"
    fi

    if [[ $PKG_YAML_CPP_DEV -eq 0 ]]; then
        logE "==  Missing        : libyaml-cpp-dev"
        PKG_MISSING="$PKG_MISSING libyaml-cpp-dev"
    else
        logP "==  Installed      : libyaml-cpp-dev"
    fi

    if [ ! -z "$PKG_MISSING" ]; then
        logP "== Installing missing packages"
        sudo apt-get install $PKG_MISSING

        testForError
    fi
}

function buildQtCreator {
    logP "=="
    logP "== Entering $QTC_SOURCE source path"

    # Clone Qt Creator and build it from source
    if [ ! -d "$QTC_SOURCE" ]; then
        logP "==  Connecting to github.com"
        cd $BASE_PATH
        cloneQtCreator
    else
        logP "==  Updating Qt Creator from github.com"
        cd $QTC_SOURCE
        pullQtCreator
    fi

    if [ ! -d $QTC_BUILD ]; then
        logP "==  Creating $QTC_BUILD build path"
        mkdir -p $QTC_BUILD && cd $QTC_BUILD
    else
        logP "==  Entering $QTC_BUILD build path"
        cd $QTC_BUILD
        if [[ $QTC_SKIP_CLEAN -eq 0 ]]; then
            build QtCreator clean
        fi
    fi

    local CMD

    if ([ "$RUN_TYPE" == "-u" ] || [ "$RUN_TYPE" == "-d" ]); then
        CMD="$QMAKE_PATH $QTC_SOURCE/qtcreator.pro -r"
    else
        CMD="$QMAKE_PATH $QTC_SOURCE/qtcreator.pro -r CONFIG+=qml_debug CONFIG+=force_debug_info CONFIG+=separate_debug_info"
    fi

    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"

    $CMD &>> "$LOG_FILE"

    testForError

    # Build QT creator
    build QtCreator
}

function buildROSQtCreatorPlugin {
    logP "=="
    logP "== Entering $QTR_SOURCE"

    local CMD
    # Build ROS Qt Creator Plugin
    if ([ "$RUN_TYPE" == "-u" ] || [ "$RUN_TYPE" == "-ui" ]); then
        if [ ! -d "$QTR_SOURCE" ]; then
            cloneROSQtPlugin
        else
            pullROSQtPlugin
        fi
    fi

    logP "==  Updating modules"

    cd $BASE_PATH/$QTR_DIR_NAME

    CMD="git submodule update --init --recursive"
    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"
    $CMD &>> "$LOG_FILE"

    CMD="git submodule foreach git fetch"
    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"
    $CMD &>> "$LOG_FILE"

    CMD="git submodule foreach git pull"
    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"
    $CMD &>> "$LOG_FILE"

    if [ ! -d $QTR_BUILD ]; then
        logP "==  Creating $QTR_BUILD build path"
        mkdir -p $QTR_BUILD && cd $QTR_BUILD
    else
        logP "==  Entering $QTR_BUILD build path"
        cd $QTR_BUILD
        if [[ $QTR_SKIP_CLEAN -eq 0 ]]; then
            build ros_qtc_plugin clean
        fi
    fi

    if ([ "$RUN_TYPE" == "-u" ] || [ "$RUN_TYPE" == "-d" ]); then
        CMD="$QMAKE_PATH $QTR_SOURCE/ros_qtc_plugin.pro -r"
    else
        CMD="$QMAKE_PATH $QTR_SOURCE/ros_qtc_plugin.pro -r CONFIG+=qml_debug CONFIG+=force_debug_info CONFIG+=separate_debug_info"
    fi

    [[ $VERBOSE -eq 1 ]] && logV "==  $CMD"

    $CMD &>> "$LOG_FILE"

    testForError

    # Build QT creator
    build ros_qtc_plugin
}

function finalStep {
    # Create desktop launch icon
    rm -f $DESKTOP_FILE
    > $DESKTOP_FILE
    echo '#!/usr/bin/env xdg-open' >> $DESKTOP_FILE
    echo '' >> $DESKTOP_FILE
    echo '[Desktop Entry]' >> $DESKTOP_FILE
    echo 'Version=4.0' >> $DESKTOP_FILE
    echo 'Encoding=UTF-8' >> $DESKTOP_FILE
    echo 'Type=Application' >> $DESKTOP_FILE
    echo 'Name=QtCreator' >> $DESKTOP_FILE
    echo 'Comment=Qt-Creator-'${QTC_BRANCH}' ROS' >> $DESKTOP_FILE
    echo 'NoDisplay=true' >> $DESKTOP_FILE
    echo 'Exec='$QTC_BUILD/bin/qtcreator >> $DESKTOP_FILE
    echo 'Icon=QtProject-qtcreator' >> $DESKTOP_FILE
    echo 'Name[en_US]=Qt-Creator-'${QTC_BRANCH}' ROS'  >> $DESKTOP_FILE
    chmod +x $DESKTOP_FILE

    logP "== Adding QtCreator-${QTC_BRANCH}.desktop to desktop"
    rm -f $HOME/Desktop/QtCreator-${QTC_BRANCH}.desktop
    ln -s $DESKTOP_FILE $HOME/Desktop/QtCreator-${QTC_BRANCH}.desktop

    testForError

    # Create user command line launch
    logP "== Adding QtCreator-${QTC_BRANCH}-ROS command line launcher: /usr/local/bin/qtcreator-${QTC_BRANCH}-ROS"
    sudo rm -f /usr/local/bin/qtcreator-${QTC_BRANCH}-ROS

    sudo ln -s $QTC_BUILD/bin/qtcreator /usr/local/bin/qtcreator-${QTC_BRANCH}-ROS

    testForError
}

deleteLog

if [ $# -eq 0 ]; then
    logGitHash
    printUsage
fi

while [ "$#" -gt 0 ]; do
  case "$1" in
    -u)             RUN_TYPE="$1";   shift 1;;
    -d)             RUN_TYPE="$1";   shift 1;;
    -ui)            RUN_TYPE="$1";   shift 1;;
    -di)            RUN_TYPE="$1";   shift 1;;
    -qtm)           QMAKE_PATH="$2"; shift 2;;
    -qtb)           QTC_BRANCH="$2"; shift 2;;
    -no-clean)      QTC_SKIP_CLEAN=1; QTR_SKIP_CLEAN=1; shift 1;;
    -no-qtc-clean)  QTC_SKIP_CLEAN=1; shift 1;;
    -no-qtr-clean)  QTR_SKIP_CLEAN=1; shift 1;;
    -qtp)           QTC_PATH="$2";   shift 2;;
    -v)             VERBOSE=1;       shift 1;;
    *)              logGitHash
                    logE "== Unknown $1 parameter!!!";
                    printUsage;;
esac
done

# Check minimum required options
if ([ "$RUN_TYPE" != "-u" ] &&
    [ "$RUN_TYPE" != "-ui" ] &&
    [ "$RUN_TYPE" != "-d" ] &&
    [ "$RUN_TYPE" != "-di" ]); then
    logGitHash
    printUsage
fi

# Change to working folder where we did run setup.sh from
cd $QTR_DIR_PATH
logP "== Output redirected to setup.log"
setParameters
logGitHash
checkPkgDependency
checkParameters
logOSInfo
logEnvInfo
logRQTEnvironment

if [ -z $QTC_PATH ]; then
    buildQtCreator
fi

buildROSQtCreatorPlugin
finalStep

logP "=="
logP "== Success!!! Happy ROSing"
logP "=="
# Go back to original location
cd $INIT_CWD &> /dev/null
