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
    # Get Major Version
	PVersion=(`echo $QTC_MINOR_VERSION | tr '.' ' '`)
	QTC_MAJOR_VERSION=${PVersion[0]}.${PVersion[1]}

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
    rm -rf /tmp/qtcreator
    rm -rf /tmp/qtcreator_dev
    rm -rf /tmp/qtcreator_ros_plugin
}

function createConfig {
    mkdir -p $INSTALLER_DIR_PATH/$INSTALL_DIR/config/
    cp $INSTALLER_DIR_PATH/logo.png $INSTALLER_DIR_PATH/$INSTALL_DIR/config/logo.png
    cp $INSTALLER_DIR_PATH/watermark.png $INSTALLER_DIR_PATH/$INSTALL_DIR/config/watermark.png

cat > $INSTALLER_DIR_PATH/$INSTALL_DIR/config/config.xml << EOF
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
                 <Url>https://aeswiki.datasys.swri.edu/qtcreator_ros/downloads/repositories/$INSTALL_DIR</Url>
                 <Enabled>1</Enabled>
                 <DisplayName>Qt Creator with ROS Plug-in</DisplayName>
         </Repository>
    </RemoteRepositories>
</Installer>
EOF
}

function createRootPackage {
    mkdir -p $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta
    cp $INSTALLER_DIR_PATH/LICENSE.GPL3-EXCEPT $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/LICENSE.GPL3-EXCEPT
    cp $INSTALLER_DIR_PATH/LICENSE.APACHE $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/LICENSE.APACHE
    cp $INSTALLER_DIR_PATH/page.ui $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/page.ui
 
cat > $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME/meta/package.xml << EOF
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
    mkdir -p $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/meta
    mkdir -p $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data

cat > $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/meta/package.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Package>
    <DisplayName>$QTC_DISPLAY_NAME</DisplayName>
    <Description>Installs the Qt Creator IDE</Description>
    <Version>$QTC_MINOR_VERSION</Version>
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
    mkdir -p $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/meta
    mkdir -p $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data

cat > $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/meta/package.xml << EOF
<?xml version="1.0" encoding="UTF-8"?>
<Package>
    <DisplayName>ROS Plug-in ($RQTC_MINOR_VERSION)</DisplayName>
    <Description>Installs the ROS Qt Creator Plug-in</Description>
    <Version>$RQTC_MINOR_VERSION</Version>
    <ReleaseDate>$RQTC_RELEASE_DATE</ReleaseDate>
    <Name>$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc</Name>
    <Dependencies>$BASE_PACKAGE_NAME.$PACKAGE_NAME</Dependencies>
    <Checkable>false</Checkable>
</Package>
EOF
}

function createInstallScript {
cat > $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/meta/installscript.qs << EOF
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

function createInstallerData {
	export QTC_SOURCE=/tmp/$INSTALL_DIR
	export QTC_BUILD=/tmp/$INSTALL_DIR

	mkdir -p /tmp/$INSTALL_DIR
	cd /tmp/$INSTALL_DIR

    # Download the version of Qt Creator from Qt
	wget https://download.qt.io/official_releases/qtcreator/$QTC_MAJOR_VERSION/$QTC_MINOR_VERSION/installer_source/linux_gcc_64_rhel72/qtcreator.7z
	
    # Extract Qt Creator Data
	7zr x -bd qtcreator.7z
    rm qtcreator.7z 

    # May need to create a symbolic link with version of qtcreator binary to allow multiple versions.

    # Package Qt Creator
    rm $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data/qtcreator.7z
	cd /tmp
	7zr a -r qtcreator.7z $INSTALL_DIR
	mv qtcreator.7z $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data

    cd /tmp/$INSTALL_DIR
    wget https://download.qt.io/official_releases/qtcreator/$QTC_MAJOR_VERSION/$QTC_MINOR_VERSION/installer_source/linux_gcc_64_rhel72/qtcreator_dev.7z
	7zr x -y -bd qtcreator_dev.7z
    rm qtcreator_dev.7z
	 
	# Clone the ROS Qt Creator Plugin
	cd /tmp
	git clone --depth 1 --single-branch --branch $QTC_MAJOR_VERSION https://github.com/ros-industrial/ros_qtc_plugin.git
    
    # Build the ROS Qt Creator Plugin
	mkdir ros_qtc_plugin-build
	cd /tmp/ros_qtc_plugin-build
	$QMAKE_PATH ../ros_qtc_plugin/ros_qtc_plugin.pro -r 
	make -j8

	# Next we must change the rpath to use the local Qt Libraries that get copied into the Qt Creator Directory
	chrpath -r \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib /tmp/$INSTALL_DIR/lib/qtcreator/plugins/libROSProjectManager.so  

    # Package ROS Qt Creator Plugin
    rm $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data/qtcreator_ros_plugin.7z
	mkdir -p /tmp/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator/plugins
	cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator/plugins
	cp /tmp/$INSTALL_DIR/lib/qtcreator/plugins/libROSProjectManager.so .
	mkdir -p /tmp/qtcreator_ros_plugin/$INSTALL_DIR/share/qtcreator
	cd /tmp/qtcreator_ros_plugin/$INSTALL_DIR/share/qtcreator
	cp -r /tmp/ros_qtc_plugin/share/styles .
	cp -r /tmp/ros_qtc_plugin/share/templates .
	cd /tmp/qtcreator_ros_plugin
	7zr a -r qtcreator_ros_plugin.7z $INSTALL_DIR
	mv qtcreator_ros_plugin.7z $INSTALLER_DIR_PATH/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data
}

function createInstaller {
    logP "Create Installer data for version $QTC_MINOR_VERSION"

    init

    createConfig
    createRootPackage

    createQtCreatorPackage
    createROSQtCreatorPluginPackage
    createInstallScript

    createInstallerData

    # Create binary 
    cd $INSTALLER_DIR_PATH/$INSTALL_DIR
    /home/larmstrong/QtIFW-3.0.2/bin/binarycreator -f -c config/config.xml -p packages qtcreator-ros-$PACKAGE_NAME-offline-installer.run
    /home/larmstrong/QtIFW-3.0.2/bin/binarycreator -n -c config/config.xml -p packages qtcreator-ros-$PACKAGE_NAME-online-installer.run

    logP "Finished Creating Installer data for version $QTC_MINOR_VERSION"
}

