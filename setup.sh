#!/bin/bash
if [ "$#" != 1 ] || ([ "$1" != "-u" ] && [ "$1" != "-ui" ] && [ "$1" != "-d" ] && [ "$1" != "-di" ]); then
    echo "Usage: bash setup.sh argument"
    echo "Arguments:"
    echo "    -u  : Run setup for users"
    echo "    -d  : Run setup for developers"
    echo "    -ui : Run setup for users with debug info"
    echo "    -di : Run setup for developers with debug info"
    exit 85
fi

# Install build dependencies
echo "Install build dependencies: build-essential libgl1-mesa-dev"
sudo apt-get install build-essential libgl1-mesa-dev

if (([ "$1" == "-u" ] || [ "$1" == "-ui" ]) && [ $(basename "$PWD") != 'ros_qtc_plugin' ]) ; then
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

# Clone Qt Creator and build it from source
if [ ! -d "$QTC_SOURCE" ]; then 
    cd $BASE_PATH && git clone -b 4.0 https://github.com/qtproject/qt-creator.git
else
    cd $BASE_PATH/qt-creator && git fetch && git pull
fi 
mkdir -p $QTC_BUILD
cd $QTC_BUILD && make clean
if ([ "$1" == "-u" ] || [ "$1" == "-d" ]); then
  cd $QTC_BUILD && qmake $QTC_SOURCE/qtcreator.pro -r
else
  cd $QTC_BUILD && qmake $QTC_SOURCE/qtcreator.pro -r CONFIG+=qml_debug CONFIG+=force_debug_info CONFIG+=separate_debug_info
fi
cd $QTC_BUILD && make -j8

# Build ROS Qt Creator Plugin
if ([ "$1" == "-u" ] || [ "$1" == "-ui" ]); then
    if [ ! -d "$ROS_SOURCE" ]; then 
        cd $BASE_PATH && git clone --recursive -b master https://github.com/ros-industrial/ros_qtc_plugin.git
    else
        cd $BASE_PATH/ros_qtc_plugin && git fetch && git pull
    fi 
fi
cd $BASE_PATH/ros_qtc_plugin && git submodule update --init --recursive
cd $BASE_PATH/ros_qtc_plugin && git submodule foreach git fetch
cd $BASE_PATH/ros_qtc_plugin && git submodule foreach git pull
mkdir -p $ROS_BUILD
cd $ROS_BUILD && make clean
if ([ "$1" == "-u" ] || [ "$1" == "-d" ]); then
  cd $ROS_BUILD && qmake $ROS_SOURCE/ros_qtc_plugin.pro -r
else
  cd $ROS_BUILD && qmake $ROS_SOURCE/ros_qtc_plugin.pro -r CONFIG+=qml_debug CONFIG+=force_debug_info CONFIG+=separate_debug_info
fi
cd $ROS_BUILD && make -j8

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
echo 'NoDsiplay=true' >> $DESKTOP_FILE
echo 'Exec='$QTC_BUILD/bin/qtcreator >> $DESKTOP_FILE
echo 'Icon=QtProject-qtcreator' >> $DESKTOP_FILE
echo 'Name[en_US]=Qt-Creator' >> $DESKTOP_FILE
chmod +x $DESKTOP_FILE 

rm -f $HOME/Desktop/QtCreator.desktop
ln -s $DESKTOP_FILE $HOME/Desktop/QtCreator.desktop

# Create user command line launch
echo 'Add Qt Creator ROS command line launcher: /usr/local/bin/qtcreator'
sudo rm -f /usr/local/bin/qtcreator 
sudo ln -s $QTC_BUILD/bin/qtcreator /usr/local/bin/qtcreator

