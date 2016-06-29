/**
 * @author Levi Armstrong
 * @date January 1, 2016
 *
 * @copyright Copyright (c) 2016, Southwest Research Institute
 *
 * @license Software License Agreement (Apache License)\n
 * \n
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at\n
 * \n
 * http://www.apache.org/licenses/LICENSE-2.0\n
 * \n
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROSUTILS_H
#define ROSUTILS_H

#include <QProcess>
#include <QXmlStreamWriter>

namespace Utils {
class FileName;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSUtils
{
public:
  ROSUtils();

  /**
   * @brief The FolderContent struct used to store file and folder information
   */
  struct FolderContent
  {
    QStringList files;
    QStringList directories;
  };

  /**
   * @brief generateCodeBlocksProjectFile - Generates a CodeBlocks project file from which include directories are extracted.
   * @param process - QProcess to execute the CMAKE command
   * @param sourceDir - Source directory of the workspace
   * @param buildDir - Build directory of the workspace
   * @return True if successful
   */
  static bool generateCodeBlocksProjectFile(QProcess *process, const Utils::FileName &sourceDir, const Utils::FileName &buildDir);

  /**
   * @brief sourceROS - Source ROS
   * @param process - QProcess to execute the ROS bash command
   * @param rosDistribution - ROS distribution
   * @return True if successful
   */
  static bool sourceROS(QProcess *process, const QString &rosDistribution);

  /**
   * @brief sourceWorkspace - Source Workspace
   * @param process - QProcess to execute the ROS bash command
   * @param workspaceDir - Workspace directory
   * @param rosDistribution - ROS distribution
   * @return True if successful
   */
  static bool sourceWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution);

  /**
   * @brief isWorkspaceInitialized - Check whether provided workspace has been initialized
   * @param workspaceDir - Workspace directory
   * @return True if workspace is initialized
   */
  static bool isWorkspaceInitialized(const Utils::FileName &workspaceDir);

  /**
   * @brief hasDevelDirectory - Check whether provided workspace has a devel directory
   * @param workspaceDir - Workspace directory
   * @return True if devel directory exists
   */
  static bool hasDevelDirectory(const Utils::FileName &workspaceDir);

  /**
   * @brief hasBuildDirectory - Check whether provided workspace has a build directory
   * @param workspaceDir - Workspace directory
   * @return True if build directory exists
   */
  static bool hasBuildDirectory(const Utils::FileName &workspaceDir);

  /**
   * @brief initializeWorkspace - Initialize workspace
   * @param process - QProcess to execute the ROS bash command
   * @param workspaceDir - Workspace directory
   * @param rosDistribution - ROS Distribution
   * @return True if successfully executed
   */
  static bool initializeWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution);

  /**
   * @brief buildWorkspace - Build workspace
   * @param process - QProcess to execute the catkin_make
   * @param workspaceDir - Workspace Directory
   * @return True if successfully executed
   */
  static bool buildWorkspace(QProcess *process, const Utils::FileName &workspaceDir);

  /**
   * @brief installedDistributions - Gets a list of installed ROS Distributions
   * @return QStringList of installed ROS Distribution Names
   */
  static QStringList installedDistributions();

  /**
   * @brief gererateQtCreatorWorkspaceFile - Generates/Updates the Qt ROS Project File
   * @param file - The Qt ROS Project Files
   * @param files - List of Workspace Files
   * @param includePaths - List of required include directories
   * @return A bool whether the Qt ROS Project file was succesfully updated
   */
  static bool gererateQtCreatorWorkspaceFile(QXmlStreamWriter &file, const QStringList &watchDirectories, const QStringList &includePaths);

  /**
   * @brief getWorkspaceFiles - Gets all fo the files in a ROS Workspace
   * @param workspaceDir - Path to the ROS Workspace
   * @return QHash<QString, QStringList> Directory, List of file paths
   */
  static QHash<QString, QStringList> getWorkspaceFiles(const Utils::FileName &workspaceDir);

  /**
   * @brief getFolderContent - Gets all fo the files in a given folder
   * @param folderPath - Path to the foder
   * @return QHash<QString, QStringList> Directory, List of file paths
   */
  static QHash<QString, FolderContent> getFolderContent(const Utils::FileName &folderPath, QStringList &fileList);

  /**
   * @brief getWorkspaceIncludes - Gets all of the include directories
   * @param workspaceDir - Path of the ROS Workspace
   * @param rosDistribution - ROS Distribution
   * @return QStringList of include directories
   */
  static QStringList getWorkspaceIncludes(const Utils::FileName &workspaceDir, const QString &rosDistribution);

  /**
   * @brief getROSPackages - Executes the bash command "rospack list" and returns
   * a map of QMap(Package Name, Path to package)
   * @param env - Is the environment to use for getting the list of available packages.
   * @return QMap(Package Name, Path to package)
   */
  static QMap<QString, QString> getROSPackages(const QStringList &env);

  /**
   * @brief getROSPackageLaunchFiles - Gets all launch file associated to a package
   * @param packagePath - ROS Package Name
   * @param OnlyNames - Flag to determine if you only need the file name versus the path
   * @return QStringList of launch files
   */
  static QStringList getROSPackageLaunchFiles(const QString &packagePath, bool OnlyNames = true);

  /**
   * @brief getROSPackageExecutables - Gets all of the executables associated to a package
   * @param packageName - ROS Package Name
   * @param env - ROS Workspace Environment
   * @return QStringList of executables
   */
  static QStringList getROSPackageExecutables(const QString &packageName, const QStringList &env);

private:

  /**
   * @brief sourceWorkspaceHelper - Source workspace helper function
   * @param process - QProcess to execute source bash command
   * @param path - Path to workspace setup.bash
   * @return True if successful
   */
  static bool sourceWorkspaceHelper(QProcess *process, const QString &path);

};

}
}

#endif // ROSUTILS_H
