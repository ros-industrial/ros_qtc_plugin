#!/bin/bash

# Verbosity level
VERBOSE=0
CPU_CORES=`grep -c ^processor /proc/cpuinfo`
# User option (-u | -d | -ud | -di)
RUN_TYPE=""
# Space seletated missing packages to be installed
PKG_MISSING=""
QMAKE_PATH="/opt/qt57/bin/qmake"
# ROS qtc plubin default branch
QTP_BRANCH="master"
# QT Creator default branch
QTC_BRANCH="4.2"
# By default clean all on rebuild
QTC_SKIP_CLEAN=0
QTC_PATH=""
# Log file of all actions
LOG_FILE="setup.log"
# Get Git hash for logging purposes
GIT_HASH=`git log --pretty=format:'%h' -n 1`

GIT_QUIET="--quiet"

function printUsage {
    echo "Usage: bash setup.sh argument"
    echo "Arguments:"
    echo "  -u       : run setup for users"
    echo "  -d       : run setup for developers"
    echo "  -ui      : run setup for users with debug info"
    echo "  -di      : run setup for developers with debug info"
    echo "  -qtb tag : build qt creator with branch/tag"
    echo "  -qtp path: qtcreator path. If provided -qtb is unused"
    echo "  -qtm path: qmake path"
    echo "  -qtc     : skip make clean. Default clean"
    echo "  -v       : verbose mode"
    echo "Defaults"
    echo "  QTCreator : $QTC_BRANCH"
    echo "  ROS Plugin: $QTP_BRANCH"
    echo "  QMake     : $QMAKE_PATH"
    echo "  Verbose   : $VERBOSE"
    exit 85
}

function deleteLog {
    rm -rf "$LOG_FILE"
}

function logE {
    echo "$1" | tee -a "$LOG_FILE"
}

function logP {
    printf "$1" | tee -a "$LOG_FILE"
}

function cloneQtCreator {
    logE "==  Cloning QT Creator $QTC_BRANCH. Stand by..."

    local CMD=" git clone $GIT_QUIET --depth 1 --single-branch --branch $QTC_BRANCH https://github.com/qtproject/qt-creator.git"

    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"

    $CMD &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "== Operation canceled"
            exit $?
    fi
}

function pullQtCreator {
    QPATH=$(basename "$PWD")
    logE "==  Fetching into $QPATH"
    git $GIT_QUIET fetch &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "== Operation canceled"
        exit $?
    fi

    logE "==  Pulling into $QPATH"
    git $GIT_QUIET pull &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "== Operation canceled"
        exit $?
    fi
}

function cloneROSQtPlugin {
    cd $BASE_PATH
    logE "==  Cloning ROS QTC Plugin($QTP_BRANCH). Stand by..."
    local CMD="git clone -b $QTP_BRANCH https://github.com/ros-industrial/ros_qtc_plugin.git"

    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"

    $CMD &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "==  Operation canceled"
            exit $?
    fi
}

function pullROSQtPlugin {
    logE "== Entering $BASE_PATH/ros_qtc_plugin"
    cd $BASE_PATH/ros_qtc_plugin

    QPATH=$(basename "$PWD")
    logE "==  Fetching into $QPATH"

    git $GIT_QUIET fetch &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "==  Operation canceled"
        exit $?
    fi

    logE "==  Pulling into $QPATH"
    git $GIT_QUIET pull &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "==  Operation canceled"
        exit $?
    fi
}

function build {
    if [ "$1" == "clean" ]; then
        logE "== Cleaning project"

        if [ -e "Makefile" ]; then
            make clean &>> "$LOG_FILE"
        fi
    else
        logE "== Building project"

        if [ -e "Makefile" ]; then
            make -j$CPU_CORES &>> "$LOG_FILE"
        fi
    fi

    if [ $? -gt 0 ]; then
        logE "== ERROR: Build failed: $?"
            exit $?
    fi
}

function logGitHash {
    logE "== ros_qtc_plugin git Hash $GIT_HASH"
}

function setParameters {
    if (([ "$1" == "-u" ] || [ "$1" == "-ui" ]) &&
         [ $(basename "$PWD") != 'ros_qtc_plugin' ]); then
        BASE_PATH=$PWD/qtc_plugins
        mkdir -p $BASE_PATH
    else
        BASE_PATH=$(dirname "$PWD")
    fi

    QTC_BUILD=$BASE_PATH/qt-creator-build
    QTC_SOURCE=$BASE_PATH/qt-creator

    ROS_BUILD=$BASE_PATH/ros_qtc_plugin-build
    ROS_SOURCE=$BASE_PATH/ros_qtc_plugin

    DESKTOP_FILE=$HOME/.local/share/applications/Qt-Creator-ros.desktop

    LOG_FILE="$ROS_SOURCE/$LOG_FILE"
}

function checkParameters {
    if [ -z "$QMAKE_PATH" ]; then
        QMAKE_PATH="/opt/qt57/bin/qmake"
    fi

    if [ ! -e "$QMAKE_PATH" ]; then
        logE "== $QMAKE_PATH is mising !!!"
        exit -1
    fi

    if [ ! -x "$QMAKE_PATH" ]; then
        logE "== $QMAKE_PATH is not executable !!!"
        exit -1
    fi
}

function logEnvironment {
    if [[ $VERBOSE -eq 1 ]]; then
        GIT_QUIET=""
        logE "== Enviroment Variables"
        logE "== BASE_PATH      : $BASE_PATH"
        logE "== LOG_FILE       : $LOG_FILE"
        logE "== QMAKE_PATH     : $QMAKE_PATH"
        logE "== QTC_SKIP_CLEAN : $QTC_SKIP_CLEAN"
        logE "== QTC_BRANCH     : $QTC_BRANCH"
        logE "== QTC_PATH       : $QTC_PATH"
        logE "== QTC_BUILD      : $QTC_BUILD"
        logE "== QTC_SOURCE     : $QTC_SOURCE"
        logE "== QTP_BRANCH     : $QTP_BRANCH"
        logE "== ROS_BUILD      : $ROS_BUILD"
        logE "== ROS_SOURCE     : $ROS_SOURCE"
        logE "== DESKTOP_FILE   : $DESKTOP_FILE"
    fi
}

function checkPkgDependency {
    # Install build dependencies
    logE "== Checking build dependencies"

    PKG_BLD_ESSENTIAL=$(dpkg-query -W --showformat='${Status}\n'\
                        build-essential 2>/dev/null | grep -c "ok installed")
    PKG_MESA_DEV=$(dpkg-query -W --showformat='${Status}\n'\
                        libgl1-mesa-dev 2>/dev/null | grep -c "ok installed")
    PKG_QT_TERM=$(dpkg-query -W --showformat='${Status}\n'\
                        libqtermwidget57-0-dev 2>/dev/null | grep -c "ok installed")
    PKG_CATKIN_TOOLS=$(dpkg-query -W --showformat='${Status}\n'\
                        python-catkin-tools 2>/dev/null | grep -c "ok installed")

    if [[ $PKG_BLD_ESSENTIAL -eq 0 ]]; then
        logP "== Missing        : "
        PKG_MISSING="$PKG_MISSING build-essential"
    else
        logP "== Installed      : "
    fi
    logE "build-essential"

    if [[ $PKG_MESA_DEV -eq 0 ]]; then
        logP "== Missing        : "
        PKG_MISSING="$PKG_MISSING libgl1-mesa-dev"
    else
        logP "== Installed      : "
    fi
    logE "libgl1-mesa-dev"

    if [[ $PKG_QT_TERM -eq 0 ]]; then
        logP "== Missing        : "
        PKG_MISSING="$PKG_MISSING libqtermwidget57-0-dev"
    else
        logP "== Installed      : "
    fi
    logE "libqtermwidget57-0-dev"

    if [[ $PKG_CATKIN_TOOLS -eq 0 ]]; then
        logP "== Missing        : "
        PKG_MISSING="$PKG_MISSING python-catkin-tools"
    else
        logP "== Installed      : "
    fi
    logE "python-catkin-tools"

    if [ ! -z "$PKG_MISSING" ]; then
        logE "== Installing missing packages"
        sudo apt-get install $PKG_MISSING

        if [ $? -gt 0 ]; then
            logE "== Operation canceled"
                exit $?
        fi
    fi
}

function buildQtCreator {
    # Clone Qt Creator and build it from source
    if [ ! -d "$QTC_SOURCE" ]; then
        logE "== Cloning Qt Creator($QTC_BRANCH) from github.com"
        cd $BASE_PATH
        cloneQtCreator
    else
        logE "== Fetching/Pulling Qt Creator from github.com"
        cd $BASE_PATH/qt-creator
        pullQtCreator
    fi

    if [ ! -d $QTC_BUILD ]; then
        logE "== Creating $QTC_BUILD build path"
        mkdir -p $QTC_BUILD && cd $QTC_BUILD
    else
        logE "== Entering $QTC_BUILD build path"
        cd $QTC_BUILD
        if [[ $QTC_SKIP_CLEAN -eq 0 ]]; then
            build clean
        fi
    fi

    local CMD

    if ([ "$RUN_TYPE" == "-u" ] || [ "$RUN_TYPE" == "-d" ]); then
        CMD="$QMAKE_PATH $QTC_SOURCE/qtcreator.pro -r"
    else
        CMD="$QMAKE_PATH $QTC_SOURCE/qtcreator.pro -r CONFIG+=qml_debug CONFIG+=force_debug_info CONFIG+=separate_debug_info"
    fi

    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"

    $CMD &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "== ERROR: Last operation returned $?"
        exit $?
    fi

    # Build QT creator
    build
}

function buildROSQtCreatorPlugin {
    local CMD
    # Build ROS Qt Creator Plugin
    if ([ "$RUN_TYPE" == "-u" ] || [ "$RUN_TYPE" == "-ui" ]); then
        if [ ! -d "$ROS_SOURCE" ]; then
            cloneROSQtPlugin
        else
            pullROSQtPlugin
        fi
    fi

    logE "== Entering $BASE_PATH/ros_qtc_plugin"
    logE "== Updating modules"

    cd $BASE_PATH/ros_qtc_plugin

    CMD="git submodule update --init --recursive"
    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"
    $CMD &>> "$LOG_FILE"

    CMD="git submodule foreach git fetch"
    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"
    $CMD &>> "$LOG_FILE"

    CMD="git submodule foreach git pull"
    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"
    $CMD &>> "$LOG_FILE"

    [[ $VERBOSE -eq 1 ]] && logE "== Creating $ROS_BUILD"
    mkdir -p $ROS_BUILD

    cd $ROS_BUILD

    if [[ $QTC_SKIP_CLEAN -eq 0 ]]; then
        build clean
    fi

    if ([ "$RUN_TYPE" == "-u" ] || [ "$RUN_TYPE" == "-d" ]); then
        CMD="$QMAKE_PATH $ROS_SOURCE/ros_qtc_plugin.pro -r"
    else
        CMD="$QMAKE_PATH $ROS_SOURCE/ros_qtc_plugin.pro -r CONFIG+=qml_debug CONFIG+=force_debug_info CONFIG+=separate_debug_info"
    fi

    [[ $VERBOSE -eq 1 ]] && logE "== $CMD"

    $CMD &>> "$LOG_FILE"

    if [ $? -gt 0 ]; then
        logE "== ERROR: Last operation returned $?"
            exit $?
    fi

    # Build QT creator
    build
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
    echo 'Comment=QtCreator' >> $DESKTOP_FILE
    echo 'NoDisplay=true' >> $DESKTOP_FILE
    echo 'Exec='$QTC_BUILD/bin/qtcreator >> $DESKTOP_FILE
    echo 'Icon=QtProject-qtcreator' >> $DESKTOP_FILE
    echo 'Name[en_US]=Qt-Creator' >> $DESKTOP_FILE
    chmod +x $DESKTOP_FILE

    logE "== Add Qt Creator to desktop"
    rm -f $HOME/Desktop/QtCreator.desktop
    ln -s $DESKTOP_FILE $HOME/Desktop/QtCreator.desktop

    # Create user command line launch
    logE "== Add Qt Creator ROS command line launcher: /usr/local/bin/qtcreator"
    sudo rm -f /usr/local/bin/qtcreator

    if [ $? -gt 0 ]; then
        logE "== Operation canceled"
            exit $?
    fi

    sudo ln -s $QTC_BUILD/bin/qtcreator /usr/local/bin/qtcreator
}

if [ $# -eq 0 ]; then
    printUsage
fi

while [ "$#" -gt 0 ]; do
  case "$1" in
    -u)    RUN_TYPE="$1";   shift 1;;
    -d)    RUN_TYPE="$1";   shift 1;;
    -ui)   RUN_TYPE="$1";   shift 1;;
    -di)   RUN_TYPE="$1";   shift 1;;
    -qtm)  QMAKE_PATH="$2"; shift 2;;
    -qtb)  QTC_BRANCH="$2"; shift 2;;
    -qtc)  QTC_SKIP_CLEAN=1; shift 1;;
    -qtp)  QTC_PATH="$2";   shift 2;;
    -v)    VERBOSE=1;       shift 1;;
    *)     logE "== Unknown $1 parameter!!!";
       printUsage;;
esac
done

# Check minimum required options
if ([ "$RUN_TYPE" != "-u" ] &&
    [ "$RUN_TYPE" != "-ui" ] &&
    [ "$RUN_TYPE" != "-d" ] &&
    [ "$RUN_TYPE" != "-di" ]); then
    printUsage
fi

setParameters
deleteLog
logGitHash
checkPkgDependency
checkParameters
logEnvironment

if [ -z $QTC_PATH ]; then
    buildQtCreator
fi

#buildROSQtCreatorPlugin
#finalStep

#logE "=="
#logE "== Success!!!"
#logE "=="
