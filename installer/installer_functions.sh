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

    # This is required to make the original rpath long so when replaced an error does not occure.
    # The replaced rpath can not be longer that the original rpath.
    BASE_PATH=/tmp/abcdefghijklmn
    mkdir -p $BASE_PATH/

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

    logP "QT_PATH=$QT_PATH"
    logP "QTC_REPO=$QTC_REPO"
    logP "QTC_MAJOR_VERSION=$QTC_MAJOR_VERSION"
    logP "QTC_MINOR_VERSION=$QTC_MINOR_VERSION"
    logP "QTC_BRANCH=$QTC_BRANCH"
    logP "RQTC_MAJOR_VERSION=$RQTC_MAJOR_VERSION"
    logP "RQTC_MINOR_VERSION=$RQTC_MINOR_VERSION"
    logP "DISTRO=$DISTRO"
    logP "PACKAGE_NAME=$PACKAGE_NAME"
    logP "INSTALL_DIR=$INSTALL_DIR"
    logP "PACKAGE_DISPLAY_NAME=$PACKAGE_DISPLAY_NAME"
    logP "QTC_DISPLAY_NAME=$QTC_DISPLAY_NAME"
    logP "CHECKBOX_DEFAULT=$CHECKBOX_DEFAULT"
    logP "COMMANDLINE_NAME=$COMMANDLINE_NAME"
    logP "OFFLINE_INSTALLER_ONLY=$OFFLINE_INSTALLER_ONLY"
    
    # remove directories that may exist
    rm -rf $BASE_PATH/$INSTALL_DIR
    rm -rf $BASE_PATH/ros_qtc_plugin
    rm -rf $BASE_PATH/ros_qtc_plugin-build
    rm -rf $BASE_PATH/qt-creator
    rm -rf $BASE_PATH/qt-creator-build
    rm -rf $BASE_PATH/qtcreator_ros_plugin
    rm -rf $BASE_PATH/qtermwidget
    rm -rf $BASE_PATH/qtermwidget-build
    rm -rf $BASE_PATH/libclang
    rm -rf $BASE_PATH/yaml-cpp-build
    rm -rf $BASE_PATH/yaml-cpp
    rm -rf $BASE_PATH/patchelf
    rm $BASE_PATH/$CLANG_FILENAME
}

function createConfig {
    mkdir -p $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/
    cp $INSTALLER_DIR_PATH/logo.png $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/logo.png
    cp $INSTALLER_DIR_PATH/watermark.png $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/config/watermark.png

    if [[ -v OFFLINE_INSTALLER_ONLY ]]; then
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
</Installer>
EOF
    else
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
                 <Url>https://qtcreator-ros.datasys.swri.edu/downloads/repositories/$DISTRO/$INSTALL_DIR</Url>
                 <Enabled>1</Enabled>
                 <DisplayName>Qt Creator with ROS Plug-in</DisplayName>
         </Repository>
    </RemoteRepositories>
</Installer>
EOF
    fi
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

    if [ "$DISTRO" = "trusty" ]; then
        # Install patchelf
        cd $BASE_PATH
        git clone https://github.com/NixOS/patchelf.git -b 0.9
        cd $BASE_PATH/patchelf
        ./bootstrap.sh
        ./configure
        make || exit 1
        make install || exit 1
    fi

    ## Required for perfparser
    apt install -y elfutils
    apt install -y libdw-dev
}

function packageSystemLibraries {

    if [ "$DISTRO" = "trusty" ]; then
        mkdir -p $BASE_PATH/$INSTALL_DIR/lib/qtcreator
        cd $BASE_PATH/$INSTALL_DIR/lib/qtcreator
        
        cp /usr/lib/x86_64-linux-gnu/libm.so .
        cp /usr/lib/x86_64-linux-gnu/libc.so .
        cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 .
        cp /usr/lib/gcc/x86_64-linux-gnu/4.9/libgcc_s.so .
        patchelf --force-rpath --set-rpath \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib libstdc++.so.6
        patchelf --force-rpath --set-rpath \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib libgcc_s.so
    fi 
}

function cloneQtCreator {
    ## Download Qt Creator from Qt and build it
    mkdir -p $BASE_PATH/$INSTALL_DIR
    cd $BASE_PATH 
    git clone $QTC_REPO -b $QTC_BRANCH
    cd $BASE_PATH/qt-creator
    git submodule update --init # This is to pull in qbs and build it.
}

function buildQtCreator {
    ## Build QtCreator
    mkdir -p $BASE_PATH/qt-creator-build
    cd $BASE_PATH/qt-creator-build
    qmake ../qt-creator/qtcreator.pro -r 
    make -j8 || exit 1
    make docs || exit 1
    make html_docs || exit 1
    make deployqt || exit 1
    make install_docs || exit 1
}

function packageQtCreator {
    # Package Qt Creator
    rm $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data/qtcreator.7z

    # Package QtCreator
    cd $BASE_PATH
    7zr a -r qtcreator.7z $INSTALL_DIR
    mv qtcreator.7z $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME/data
}

function processQtermwidget {
    # Clone the qtermwidget
    cd $BASE_PATH
    git clone --depth 1 --single-branch --branch $QTERMWIDGET_BRANCH https://github.com/Levi-Armstrong/qtermwidget.git
    
    # Build the qtermwidget
    cd $BASE_PATH
    mkdir qtermwidget-build
    cd $BASE_PATH/qtermwidget-build
    cmake -DCMAKE_CXX_FLAGS=-std=c++11 ../qtermwidget/
    make -j8 || exit 1
    make install || exit 1
    
    # Transfer qtermwedget file to correct location
    mv $BASE_PATH/qtermwidget-build/libqtermwidget5.so* $BASE_PATH/$INSTALL_DIR/lib/qtcreator
    
    # Next change the rpath to use local Qt Libraries copied into the Qt Creator Directory
    chrpath -r \$\ORIGIN/../Qt/lib $BASE_PATH/$INSTALL_DIR/lib/qtcreator/libqtermwidget5.so
}

function processYamlCpp {
    if [[ $YAML_CPP_TAG ]]; then
        # Clone the yaml-cpp
        cd $BASE_PATH
        git clone --depth 1 --single-branch --branch $YAML_CPP_TAG https://github.com/jbeder/yaml-cpp.git

        # Build yaml-cpp
        cd $BASE_PATH
        mkdir yaml-cpp-build
        cd $BASE_PATH/yaml-cpp-build
        cmake -DBUILD_SHARED_LIBS=ON ../yaml-cpp/
        make -j8 || exit 1
        make install || exit 1

        # Transfer yaml-cpp file to correct location
        mv $BASE_PATH/yaml-cpp-build/libyaml-cpp.so* $BASE_PATH/$INSTALL_DIR/lib/qtcreator
    fi
}

function processROSPlugin {
    processQtermwidget
    processYamlCpp

    # Clone the ROS Qt Creator Plugin
    cd $BASE_PATH
    git clone --depth 1 --single-branch --branch $QTC_MAJOR_VERSION https://github.com/ros-industrial/ros_qtc_plugin.git
    
    # Build the ROS Qt Creator Plugin
    mkdir ros_qtc_plugin-build
    cd $BASE_PATH/ros_qtc_plugin-build
    qmake ../ros_qtc_plugin/ros_qtc_plugin.pro -r 
    make -j8 || exit 1

    # Next change the rpath to use the local Qt Libraries copied into the Qt Creator Directory
    chrpath -r \$\ORIGIN:\$\ORIGIN/..:\$\ORIGIN/../lib/qtcreator:\$\ORIGIN/../../Qt/lib $BASE_PATH/$INSTALL_DIR/lib/qtcreator/plugins/libROSProjectManager.so  
}

function packageROSPlugin {
    # Package ROS Qt Creator Plugin
    ## Add ROS plugin files
    rm $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data/qtcreator_ros_plugin.7z
    mkdir -p $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator/plugins
    cd $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator/plugins
    cp $BASE_PATH/$INSTALL_DIR/lib/qtcreator/plugins/libROSProjectManager.so .
    mkdir -p $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/share/qtcreator
    cd $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/share/qtcreator
    cp -r $BASE_PATH/ros_qtc_plugin/share/styles .
    cp -r $BASE_PATH/ros_qtc_plugin/share/templates .
    
    ## Add qtermwidget files
    cd $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator
    mv $BASE_PATH/$INSTALL_DIR/lib/qtcreator/libqtermwidget5.so* .
    ## qtermwidget looks for the color-schemes and kb-layouts directory in the same path as the executable
    mkdir -p $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/bin
    cd $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/bin
    cp -R $BASE_PATH/qtermwidget/lib/color-schemes .
    cp -R $BASE_PATH/qtermwidget/lib/kb-layouts .

    if [[ $YAML_CPP_TAG ]]; then
        ## Add yaml-cpp files
        cd $BASE_PATH/qtcreator_ros_plugin/$INSTALL_DIR/lib/qtcreator
        mv $BASE_PATH/$INSTALL_DIR/lib/qtcreator/libyaml-cpp.so* .
    fi

    # Creat install package
    cd $BASE_PATH/qtcreator_ros_plugin
    7zr a -r qtcreator_ros_plugin.7z $INSTALL_DIR
    mv qtcreator_ros_plugin.7z $INSTALLER_DIR_PATH/$DISTRO/$INSTALL_DIR/packages/$BASE_PACKAGE_NAME.$PACKAGE_NAME.rqtc/data
}

function setupEnvironment {
    export QTDIR=$QT_PATH # where you downloaded and compiled qt
    export PATH=$QTDIR/bin:$PATH
    export MANPATH=$QTDIR/doc/man:$MANPATH
    export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
    export QTC_SOURCE=$BASE_PATH/qt-creator
    export QTC_BUILD=$BASE_PATH/$INSTALL_DIR
    export LLVM_INSTALL_DIR=$BASE_PATH/libclang
    export QTC_ENABLE_CLANG_LIBTOOLING=true
    export BUILD_CPLUSPLUS_TOOLS=true
    export INSTALL_ROOT=$BASE_PATH/$INSTALL_DIR
    export ELFUTILS_INSTALL_DIR=/usr/include/elfutils
    export QT_INSTALL_DOCS=$INSTALL_ROOT/share/doc
}

function createInstallerData {
    setupEnvironment

    # Download the correct version of clang
    cd $BASE_PATH
    wget https://download.qt.io/development_releases/prebuilt/libclang/$CLANG_FILENAME
    7zr x -bd $CLANG_FILENAME

    cloneQtCreator
    buildQtCreator
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
    if [[ -v OFFLINE_INSTALLER_ONLY ]]; then
        $QIFW_PATH -f -c config/config.xml -p packages qtcreator-ros-$DISTRO-$PACKAGE_NAME-offline-installer.run
    else
        $QIFW_PATH -f -c config/config.xml -p packages qtcreator-ros-$DISTRO-$PACKAGE_NAME-offline-installer.run
        $QIFW_PATH -n -c config/config.xml -p packages qtcreator-ros-$DISTRO-$PACKAGE_NAME-online-installer.run
    fi

    logP "Finished Creating Installer data for version $QTC_MINOR_VERSION"
}

