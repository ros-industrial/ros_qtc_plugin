# Creating Installers
  It was noticed that the installers need to be created specific to each version of ubuntu. The best way to accomplish this was to use docker and this explains how.

## First you need to install all necessary version of Qt Libraries

   Download the version from [here](https://download.qt.io/official_releases/qt/) into your home directory. When deploying remember to not install QtWebEngin because it cause Qt Creator to crash when clicking help.

## Next you need to install Qt Installer frame work.
 
   Download the latest version from [here](https://download.qt.io/official_releases/qt-installer-framework/) into your home directory.

## Build Docker image for version (Optional if already done)

   `docker build -t rqtc_<distro>:latest -f dockerfiles/<distro>.dockerfile --no-cache --network host .`

## Next you will need to run the following docker command (replace <distro> with trusty, xenial, etc.).

   `docker run -it --network host -v /home/<username>/qtc_plugins/appDir:/home rqtc_<distro>:latest /bin/bash`

## Next run the bash scripts to build the installers (run commands in docker)

   ```
   cd /home/ros_qtc_plugin/installer
   bash create_latest_installer
   ```

## Next set the user and group for the folder (run command in host)

   `sudo chown -R <username>:<username> <distro>/`

## Next run the Qt update tool repgen
   
   `cd /home/<username>/qtc_plugins/appDir/ros_qtc_installer_repo/<distro>/<version>/`
   `/home/<username>/qtc_plugins/appDir/QtIFW-3.0.2/bin/repogen --update-new-components -p /home/<username>/qtc_plugins/appDir/ros_qtc_plugin/installer/<distro>/<version>/packages/ .`

## Next use the folder browser connect to server to transfer files to aeswiki.datasys.swri.edu, qtcreator_ros/downloads/repositories/<distro>/<version>/.
 
## Helpfull Commands

   `xhost +local:docker`
   `docker run -ti --rm -e DISPLAY=$DISPLAY -v /tmp/.X11-unix/:/tmp/.X11-unix -v /home/<username>:/home rqtc_<distro>:latest /bin/bash`

