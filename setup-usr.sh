#!/bin/bash

BASE_PATH=$HOME/qtc_plugins

QTC_BUILD=$BASE_PATH/qt-creator-build
QTC_SOURCE=$BASE_PATH/qt-creator

ROS_BUILD=$BASE_PATH/ros_qtc_plugins-build
ROS_SOURCE=$BASE_PATH/ros_qtc_plugins

DESKTOP_FILE=$HOME/.local/share/applications/Qt-Creator-ros.desktop

mkdir -p $BASE_PATH

# Clone Qt Creator and build it from source
cd $BASE_PATH && git clone -b 4.0 https://github.com/qtproject/qt-creator.git
mkdir -p $QTC_BUILD
cd $QTC_BUILD && qmake -r $QTC_SOURCE/qtcreator.pro
cd $QTC_BUILD && make -j8

# Clone ROS Qt Creator Plugin and build it from source
cd $BASE_PATH && git clone --recursive -b master https://github.com/Levi-Armstrong/ros_qtc_plugins.git
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
sudo ln -s $QTC_BUILD/bin/qtcreator /usr/local/bin/qtcreator-ros

