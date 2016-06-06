#!/bin/bash

# Install build dependencies
echo "Install build dependencies: build-essential libgl1-mesa-dev"
sudo apt-get install build-essential libgl1-mesa-dev

BASE_PATH=$PWD/qtc_plugins

QTC_BUILD=$BASE_PATH/qt-creator-build
QTC_SOURCE=$BASE_PATH/qt-creator

ROS_BUILD=$BASE_PATH/ros_qtc_plugins-build
ROS_SOURCE=$BASE_PATH/ros_qtc_plugins

DESKTOP_FILE=$HOME/.local/share/applications/Qt-Creator-ros.desktop
ROS_QTC_SOURCE=$HOME/.local/share/ros_qtc.source

#Set perminate environment vairable holding the location to Qt Creator source
if [ ! -f "$ROS_QTC_SOURCE" ]; then
    echo 'source '$ROS_QTC_SOURCE >> ~/.bashrc
fi
> $ROS_QTC_SOURCE
echo 'export ROS_QTC_SOURCE='$BASE_PATH >> $ROS_QTC_SOURCE
source $ROS_QTC_SOURCE

mkdir -p $BASE_PATH

# Clone Qt Creator and build it from source
if [ ! -d "$QTC_SOURCE" ]; then 
    cd $BASE_PATH && git clone -b 4.0 https://github.com/qtproject/qt-creator.git
else
    cd $BASE_PATH/qt-creator && git fetch && git pull
fi 
mkdir -p $QTC_BUILD
cd $QTC_BUILD && qmake -r $QTC_SOURCE/qtcreator.pro
cd $QTC_BUILD && make -j8

# Clone ROS Qt Creator Plugin and build it from source
if [ ! -d "$ROS_SOURCE" ]; then 
    cd $BASE_PATH && git clone --recursive -b master https://github.com/Levi-Armstrong/ros_qtc_plugins.git
else
    cd $BASE_PATH/ros_qtc_plugins && git fetch && git pull
    cd $BASE_PATH/ros_qtc_plugins && git submodule foreach git fetch
    cd $BASE_PATH/ros_qtc_plugins && git submodule foreach git pull
fi 
mkdir -p $ROS_BUILD
cd $ROS_BUILD && qmake -r $ROS_SOURCE/ros_qtc_plugins.pro
cd $ROS_BUILD && make -j8

# Create desktop launch icon
rm -f $DESKTOP_FILE 
> $DESKTOP_FILE 
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
echo 'Add Qt Creator ROS command line launcher: /usr/local/bin/qtcreator-ros'
sudo rm -f /usr/local/bin/qtcreator-ros 
sudo ln -s $DESKTOP_FILE /usr/local/bin/qtcreator-ros

