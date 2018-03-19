#!/bin/bash

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

# Log file of all actions
LOG_FILE="/tmp/qtcreator_ros_installer_setup.log"
rm $LOG_FILE

function logP {
    echo -e "${GREEN}$1${NC}"   # Print color to screen
    echo -e "$1" >> "$LOG_FILE" # No color to log file
}

# Absolute path to this setup.sh script: create_installer.sh
SCRIPT_FILE_PATH=$(readlink -f $0)
# Absolute path to this setup.sh script
INSTALLER_DIR_PATH=`dirname $SCRIPT_FILE_PATH`
BASE_PACKAGE_NAME=org.rosindustrial.qtros

function init {
    # Get Qt Creator Major and Minor Version
	PVersion=(`echo $QTC_VERSION | tr '.' ' '`)
	QTC_MAJOR_VERSION=${PVersion[0]}.${PVersion[1]}
    QTC_MINOR_VERSION=${PVersion[0]}.${PVersion[1]}.${PVersion[2]}

    # Get ROS Plugin Major and Minor Version
    RVersion=(`echo $RQTC_VERSION | tr '.' ' '`)
    RQTC_MAJOR_VERSION=${RVersion[0]}.${RVersion[1]}
    RQTC_MINOR_VERSION=${RVersion[0]}.${RVersion[1]}.${RVersion[2]}

    DISTRO=(`lsb_release -cs`)

    if [ $QTC_LATEST -eq 1 ]; then
        PACKAGE_NAME=latest
        INSTALL_DIR=latest
        PACKAGE_DISPLAY_NAME="Qt Creator (latest)"
        QTC_DISPLAY_NAME="Qt Creator ($QTC_MINOR_VERSION)"
        CHECKBOX_DEFAULT=true
        COMMANDLINE_NAME="qtcreator-ros"
    else
        PACKAGE_NAME=${PVersion[0]}${PVersion[1]}${PVersion[2]}
        INSTALL_DIR=$QTC_MINOR_VERSION
        PACKAGE_DISPLAY_NAME="Qt Creator ($QTC_MINOR_VERSION)"
        QTC_DISPLAY_NAME="Qt Creator"
        CHECKBOX_DEFAULT=false
        COMMANDLINE_NAME="qtcreator-ros-$PACKAGE_NAME"
    fi
    
    # remove directories that may exist
    rm -rf /tmp/$INSTALL_DIR
    rm -rf /tmp/ros_qtc_plugin
    rm -rf /tmp/ros_qtc_plugin-build
    rm -rf /tmp/qt-creator
    rm -rf /tmp/qt-creator-build
    rm -rf /tmp/qtcreator_ros_plugin
    rm -rf /tmp/qtermwidget
    rm -rf /tmp/qtermwidget-build
    rm -rf /tmp/libclang
    rm -rf /tmp/yaml-cpp-build
    rm -rf /tmp/yaml-cpp
    rm -rf /tmp/patchelf
    rm /tmp/$CLANG_FILENAME
}

function createConfig {
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/
    cp $INSTALLER_DIR_PATH/logo.png $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/logo.png
    cp $INSTALLER_DIR_PATH/watermark.png $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/watermark.png

cat > $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/config.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Installer>
    <Name>Qt Creator with ROS Plug-in</Name>
    <Version>$INSTALLER_VERSION</Version>
    <Title>Qt Creator with ROS Plug-in</Title>
    <Publisher>Qt Project and ROS-Industrial</Publisher>

    <InstallerWindowIcon>logo.png</InstallerWindowIcon>
    <Watermark>watermark.png</Watermark>
    <WizardDefaultHeight>520</WizardDefaultHeight>
    <MaintenanceToolName>MaintenanceTool</MaintenanceToolName>
    <TargetDir>@HomeDir@/QtCreator</TargetDir>
    <RemoteRepositories>
         <Repository>
                 <Url>https://aeswiki.datasys.swri.edu/qtcreator_ros/downloads/repositories/$DISTRO/$INSTALL_DIR</Url>
                 <Enabled>1</Enabled>
                 <DisplayName>Qt Creator with ROS Plug-in</DisplayName>
         </Repository>
    </RemoteRepositories>
</Installer>
EOF
}

function createRootPackage {
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta
    cp $INSTALLER_DIR_PATH/LICENSE.GPL3-EXCEPT $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/LICENSE.GPL3-EXCEPT
    cp $INSTALLER_DIR_PATH/LICENSE.APACHE $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/LICENSE.APACHE
    cp $INSTALLER_DIR_PATH/page.ui $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/page.ui
 
cat > $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/package.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Package>
    <DisplayName>Qt Creator for ROS</DisplayName>
    <Description>Install Qt Creator for ROS Development</Description>
    <Version>$INSTALLER_VERSION</Version>
    <ReleaseDate>$INSTALLER_RELEASE_DATE</ReleaseDate>
    <Licenses>
        <License name="GNU GPL version 3 (with exception clauses)" file="LICENSE.GPL3-EXCEPT" />
        <License name="Apache License, Version 2.0" file="LICENSE.APACHE" />
    </Licenses>
    <UserInterfaces>
        <UserInterface>page.ui</UserInterface>
    </UserInterfaces>
    <Checkable>false</Checkable>
</Package>
EOF
}

function createQtCreatorPackage {
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/meta
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data

cat > $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/meta/package.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Package>
    <DisplayName>$QTC_DISPLAY_NAME</DisplayName>
    <Description>Installs the Qt Creator IDE</Description>
    <Version>$QTC_VERSION</Version>
    <ReleaseDate>$QTC_RELEASE_DATE</ReleaseDate>
    <Name>$BASE_PACKAGE_NAME.$PACKAGE_NAME</Name>
    <Dependencies>$BASE_PACKAGE_NAME</Dependencies>
    <SortingPriority>$SortingPriority</SortingPriority>
    <Script>installscript.qs</Script>
    <Checkable>true</Checkable>
</Package>
EOF
}

function createROSQtCreatorPluginPackage {
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/meta
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data

cat > $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/meta/package.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Package>
    <DisplayName>ROS Plug-in ($RQTC_MINOR_VERSION)</DisplayName>
    <Description>Installs the ROS Qt Creator Plug-in</Description>
    <Version>$RQTC_VERSION</Version>
    <ReleaseDate>$RQTC_RELEASE_DATE</ReleaseDate>
    <Name>$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc</Name>
    <Dependencies>$BASE_PACKAGE_NAME.$PACKAGE_NAME</Dependencies>
    <Checkable>false</Checkable>
</Package>
EOF
}

function createInstallScript {
cat > $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/meta/installscript.qs << EOF
function Component()
{
}

Component.prototype.createOperations = function()
{
    // Call the base createOperations and afterwards set some registry settings
    component.createOperations();
    if ( installer.value("os") == "x11" )
    {
        component.addOperation("CreateLink", "@HomeDir@/.local/bin/$COMMANDLINE_NAME", "@TargetDir@/$INSTALL_DIR/bin/qtcreator");
        component.addOperation("CreateLink", "@TargetDir@/$INSTALL_DIR/bin/$COMMANDLINE_NAME", "@TargetDir@/$INSTALL_DIR/bin/qtcreator");


        component.addOperation( "InstallIcons", "@TargetDir@/$INSTALL_DIR/share/icons" );
        component.addOperation( "CreateDesktopEntry",
                                "QtProject-qtcreator-ros-$PACKAGE_NAME.desktop",
                                "Type=Application\nExec=@TargetDir@/$INSTALL_DIR/bin/$COMMANDLINE_NAME\nPath=@TargetDir@/$INSTALL_DIR\nName=Qt Creator ($QTC_MINOR_VERSION)\nGenericName=The IDE of choice for Qt development.\nGenericName[de]=Die IDE der Wahl zur Qt Entwicklung\nIcon=QtProject-qtcreator\nTerminal=false\nCategories=Development;IDE;Qt;\nMimeType=text/x-c++src;text/x-c++hdr;text/x-xsrc;application/x-designer;application/vnd.qt.qmakeprofile;application/vnd.qt.xml.resource;text/x-qml;text/x-qt.qml;text/x-qt.qbs;"
                                );

        component.addOperation("CreateLink", "@HomeDir@/Desktop/QtProject-qtcreator-ros-$PACKAGE_NAME.desktop", "@HomeDir@/.local/share/applications/QtProject-qtcreator-ros-$PACKAGE_NAME.desktop");
        maintenanceToolPath = "@TargetDir@/MaintenanceTool";

        var settingsFile = "@TargetDir@/$INSTALL_DIR/share/qtcreator/QtProject/QtCreator.ini";
        
        // Configure UpdateInfo plugin
        component.addOperation("Settings", "path="+settingsFile, "method=set",
                               "key=Updater/MaintenanceTool",
                               "value="+maintenanceToolPath);
        component.addOperation("Settings", "path="+settingsFile,
                               "method=add_array_value",
                               "key=Plugins/ForceEnabled", "value=UpdateInfo");
        
    }
}
EOF
}

function installDepends {

    if [[ $DISTRO -eq "trusty" ]]; then
        # Install patchelf
        cd /tmp
        git clone https://github.com/NixOS/patchelf.git -b 0.9
        cd /tmp/patchelf
        ./bootstrap.sh
        ./configure
        make
        make install
    fi

}

function packageSystemLibraries {

    if [[ $DISTRO -eq "trusty" ]]; then
        mkdir -p /tmp/$INSTALL_DIR/lib/qtcreator
        cd /tmp/$INSTALL_DIR/lib/qtcreator
        
        cp /usr/lib/x86_64-linux-gnu/libm.so .
        cp /usr/lib/x86_64-linux-gnu/libc.so .
        cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 .
        cp /usr/lib/gcc/x86_64-linux-gnu/4.9/libgcc_s.so .
        patchelf --force-rpath --set-rpath \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib libstdc++.so.6
        patchelf --force-rpath --set-rpath \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib libgcc_s.so
    fi

}

function processQtCreator {
    ## Download Qt Creator from Qt and build it
    mkdir -p /tmp/$INSTALL_DIR
    cd /tmp 
    git clone https://github.com/qt-creator/qt-creator.git -b v$QTC_MINOR_VERSION
    cd /tmp/qt-creator
    git submodule update --init # This is to pull in qbs and build it.
    mkdir -p /tmp/qt-creator-build
    cd /tmp/qt-creator-build
    qmake ../qt-creator/qtcreator.pro -r 
    make -j8
    make deployqt
}

function packageQtCreator {
    # Package Qt Creator
    rm $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data/qtcreator.7z

    # Package QtCreator
    cd /tmp
    7zr a -r qtcreator.7z $INSTALL_DIR
    mv qtcreator.7z $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data
}

function processQtermwidget {
    # Clone the qtermwidget
    cd /tmp
    git clone --depth 1 --single-branch --branch debian/$DISTRO https://github.com/Levi-Armstrong/qtermwidget.git
    
    # Build the qtermwidget
    cd /tmp 
    mkdir qtermwidget-build
    cd /tmp/qtermwidget-build
    cmake -DCMAKE_CXX_FLAGS=-std=c++11 ../qtermwidget/
    make -j8
    make install
    
    # Transfer qtermwedget file to correct location
    mv /tmp/qtermwidget-build/libqtermwidget5.so* /tmp/$INSTALL_DIR/lib/qtcreator
    
    # Next change the rpath to use local Qt Libraries copied into the Qt Creator Directory
    chrpath -r \$\ORIGIN/../Qt/lib /tmp/$INSTALL_DIR/lib/qtcreator/libqtermwidget5.so
}

function processYamlCpp {
    if [[ $YAML_CPP_TAG ]]; then
        # Clone the yaml-cpp
        cd /tmp
        git clone --depth 1 --single-branch --branch $YAML_CPP_TAG https://github.com/jbeder/yaml-cpp.git

        # Build yaml-cpp
        cd /tmp
        mkdir yaml-cpp-build
        cd /tmp/yaml-cpp-build
        cmake -DBUILD_SHARED_LIBS=ON ../yaml-cpp/
        make -j8
        make install

        # Transfer yaml-cpp file to correct location
        mv /tmp/yaml-cpp-build/libyaml-cpp.so* /tmp/$INSTALL_DIR/lib/qtcreator
    fi
}

function processROSPlugin {
    processQtermwidget
    processYamlCpp

    # Clone the ROS Qt Creator Plugin
    cd /tmp
    git clone --depth 1 --single-branch --branch $QTC_MAJOR_VERSION https://github.com/ros-industrial/ros_qtc_plugin.git
    
    # Build the ROS Qt Creator Plugin
    mkdir ros_qtc_plugin-build
    cd /tmp/ros_qtc_plugin-build
    qmake ../ros_qtc_plugin/ros_qtc_plugin.pro -r 
    make -j8

    # Next change the rpath to use the local Qt Libraries copied into the Qt Creator Directory
    chrpath -r \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib /tmp/$INSTALL_DIR/lib/qtcreator/plugins/libROSProjectManager.so  
}

function packageROSPlugin {
    # Package ROS Qt Creator Plugin
    ## Add ROS plugin files
    rm $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data/qtcreator_ros_plugin.7z
    mkdir -p /tmp/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator/plugins
    cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator/plugins
    cp /tmp/$INSTALL_DIR/lib/qtcreator/plugins/libROSProjectManager.so .
    mkdir -p /tmp/qtcreator_ros_plugin/$INSTALL_DIR/share/qtcreator
    cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/share/qtcreator
    cp -r /tmp/ros_qtc_plugin/share/styles .
    cp -r /tmp/ros_qtc_plugin/share/templates .
    
    ## Add qtermwidget files
    cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator
    mv /tmp/$INSTALL_DIR/lib/qtcreator/libqtermwidget5.so* .
    ## qtermwidget looks for the color-schemes and kb-layouts directory in the same path as the executable
    mkdir -p /tmp/qtcreator_ros_plugin/$INSTALL_DIR/bin
    cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/bin
    cp -R /tmp/qtermwidget/lib/color-schemes .
    cp -R /tmp/qtermwidget/lib/kb-layouts .

    if [[ $YAML_CPP_TAG ]]; then
        ## Add yaml-cpp files
        cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator
        mv /tmp/$INSTALL_DIR/lib/qtcreator/libyaml-cpp.so* .
    fi

    # Creat install package
    cd /tmp/qtcreator_ros_plugin
    7zr a -r qtcreator_ros_plugin.7z $INSTALL_DIR
    mv qtcreator_ros_plugin.7z $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data
}

function createInstallerData {
    export QTDIR=$QT_PATH # where you downloaded and compiled qt
    export PATH=$QTDIR/bin:$PATH
    export MANPATH=$QTDIR/doc/man:$MANPATH
    export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
	export QTC_SOURCE=/tmp/qt-creator
	export QTC_BUILD=/tmp/$INSTALL_DIR
    export LLVM_INSTALL_DIR=/tmp/libclang
    export INSTALL_ROOT=/tmp/$INSTALL_DIR

    # Download the correct version of clang
    cd /tmp
    wget https://download.qt.io/development_releases/prebuilt/libclang/$CLANG_FILENAME
    7zr x -bd $CLANG_FILENAME

    processQtCreator
    packageQtCreator

    processROSPlugin
    packageROSPlugin
}

function createInstaller {
    logP "Create Installer data for version $QTC_MINOR_VERSION"

    init

    installDepends

    # Need to package system libraries if needed like libstdc++
    packageSystemLibraries

    createConfig
    createRootPackage

    createQtCreatorPackage
    createROSQtCreatorPluginPackage
    createInstallScript

    createInstallerData

    # Create binary 
    cd $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR
    $QIFW_PATH -f -c config/config.xml -p packages qtcreator-ros-$DISTRO-$PACKAGE_NAME-offline-installer.run
    $QIFW_PATH -n -c config/config.xml -p packages qtcreator-ros-$DISTRO-$PACKAGE_NAME-online-installer.run

    logP "Finished Creating Installer data for version $QTC_MINOR_VERSION"
}

