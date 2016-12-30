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
#include <QProcessEnvironment>
#include <QXmlStreamWriter>

namespace Utils {
class FileName;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSUtils {
public:
    ROSUtils();

    enum BuildSystem { CatkinMake = 0, CatkinTools = 1 };

    enum BuildType {
        BuildTypeDebug = 0,
        BuildTypeRelease = 1,
        BuildTypeRelWithDebInfo = 2,
        BuildTypeMinSizeRel = 3
    };

    /** @brief The FolderContent struct used to store file and folder information */
    struct FolderContent {
        QStringList files;       /**< @brief Directory Files */
        QStringList directories; /**< @brief Directory Subdirectories */
    };

    /** @brief Contains all relavent package information */
    struct PackageInfo {
        QString name;         /**< @brief Package Name */
        QString path;         /**< @brief Package directory path */
        QString filepath;     /**< @brief Package package.xml filepath */
        QStringList includes; /**< @brief Package include directories */
        QStringList flags;    /**< @brief Package cxx build flags */
        QString cbpFile;      /**< @brief Path to the CodeBlocks file */

        /**
         * @brief Check if package exists.
         * @return True if exists, otherwise false.
         */
        bool exists();

        /**
         * @brief Check if CodeBlocks file exists
         * @return True if exists, otherwise false.
         */
        bool cbpFileExists();
    };

    /** @brief Contains project file information */
    struct ROSProjectFileContent {
        QString distribution;                     /**< @brief ROS Distribution */
        ROSUtils::BuildSystem defaultBuildSystem; /**< @brief Default build system */
        QStringList watchDirectories;             /**< @brief Watch directories */
    };

    /**
     * @brief Convert ENUM BuiltType to QString
     * @param buildType ENUM BuildType
     * @return QString for ENUM BuildType
     */
    static QString buildTypeName(const ROSUtils::BuildType &buildType);

    /**
     * @brief Source ROS
     * @param process QProcess to execute the ROS bash command
     * @param rosDistribution ROS distribution
     * @return True if successful, otherwise false
     */
    static bool sourceROS(QProcess *process, const QString &rosDistribution);

    /**
     * @brief Source Workspace
     * @param process QProcess to execute the ROS bash command
     * @param workspaceDir Workspace directory
     * @param rosDistribution ROS distribution
     * @param buildSystem Workspace build system
     * @return True if successful
     */
    static bool sourceWorkspace(QProcess *process, const Utils::FileName &workspaceDir,
                                const QString &rosDistribution, const BuildSystem buildSystem);

    /**
     * @brief Check whether the provided workspace has been initialized
     * @param workspaceDir Workspace directory
     * @param buildSystem Workspace build system
     * @return True if workspace is initialized, otherwise false
     */
    static bool isWorkspaceInitialized(const Utils::FileName &workspaceDir,
                                       const BuildSystem BuildSystem);

    /**
     * @brief Initialize workspace
     * @param process QProcess to execute the ROS bash command
     * @param workspaceDir Workspace directory
     * @param rosDistribution ROS Distribution
     * @param buildSystem Workspace build system
     * @return True if successfully executed, otherwise false
     */
    static bool initializeWorkspace(QProcess *process, const Utils::FileName &workspaceDir,
                                    const QString &rosDistribution, const BuildSystem buildSystem);

    /**
     * @brief Build workspace
     * @param process QProcess to execute the catkin_make
     * @param workspaceDir Workspace Directory
     * @param buildSystem Workspace build system
     * @return True if successfully executed, otherwise false
     */
    static bool buildWorkspace(QProcess *process, const Utils::FileName &workspaceDir,
                               const ROSUtils::BuildSystem buildSystem);

    /**
     * @brief Gets a list of installed ROS Distributions
     * @return QStringList List of installed ROS distribution names
     */
    static QStringList installedDistributions();

    /**
     * @brief Generates/Updates the Qt ROS Project File
     * @param content Project content to write to file
     * @return True if Qt ROS Project file was succesfully updated, otherwise false
     */
    static bool gererateQtCreatorWorkspaceFile(QXmlStreamWriter &file,
                                               const ROSProjectFileContent &content);

    /**
     * @brief Parse the Qt ROS Project File
     * @param filePath Path to the Qt ROS Project File
     * @param content Item to populate with data from file
     * @return True if Qt ROS Project file was succesfully parsed, otherwise false
     */
    static bool parseQtCreatorWorkspaceFile(const Utils::FileName &filePath,
                                            ROSProjectFileContent &content);

    /**
     * @brief Gets all fo the files in a given folder
     * @param folderPath Path to the foder
     * @param fileList List of files in directory
     * @return QHash<QString, FolderContent> Directory, FolderContent
     */
    static QHash<QString, FolderContent> getFolderContent(const Utils::FileName &folderPath,
                                                          QStringList &fileList);

    /**
     * @brief Get all of the workspace packages and its neccessary information.
     * @param workspaceDir Path of the workspace
     * @param buildSystem Workspace build system
     * @return QMap(Package Name, PackageInfo)
     */
    static QMap<QString, ROSUtils::PackageInfo>
    getWorkspacePackageInfo(const Utils::FileName &workspaceDir, const BuildSystem buildSystem);

    /**
     * @brief Executes the bash command "rospack list" and returns a map of QMap(Package Name, Path
     * to package)
     * @param env Is the environment to use for getting the list of available packages.
     * @return QMap(Package Name, Path to package)
     */
    static QMap<QString, QString> getROSPackages(const QStringList &env);

    /**
     * @brief Get all of the ros packages within the provided workspace directory.
     * @param workspaceDir Workspace directory path
     * @param buildSystem Workspace build system
     * @return QMap(Package Name, Path to package)
     */
    static QMap<QString, QString> getWorkspacePackages(const Utils::FileName &workspaceDir,
                                                       const BuildSystem buildSystem);

    /**
     * @brief Get all of the code block files within workspace build directory.
     * @param workspaceDir Workspace directory path
     * @param buildSystem Workspace build system
     * @return QMap(CodeBlock File Name, CodeBlock File Path)
     */
    static QMap<QString, QString> getWorkspaceCodeBlockFiles(const Utils::FileName &workspaceDir,
                                                             const BuildSystem buildSystem);

    /**
     * @brief Gets all launch files associated to a package
     * @param packagePath ROS Package Name
     * @param OnlyNames Flag to determine if you only need the file name versus the path
     * @return QStringList of launch files
     */
    static QStringList getROSPackageLaunchFiles(const QString &packagePath, bool OnlyNames = true);

    /**
     * @brief Gets all of the executables associated to a package
     * @param packageName ROS Package Name
     * @param env ROS Workspace Environment
     * @return QStringList of executables
     */
    static QStringList getROSPackageExecutables(const QString &packageName, const QStringList &env);

    /**
     * @brief Remove catkin tools profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return True if successful, otherwise false
     */
    static bool removeCatkinToolsProfile(const Utils::FileName &workspaceDir,
                                         const QString profileName);

    /**
     * @brief Rename catkin tools profile
     * @param workspaceDir Workspace directory path
     * @param oldProfileName Old profile name
     * @param newProfileName New profile name
     * @return True if successful, otherwise false
     */
    static bool renameCatkinToolsProfile(const Utils::FileName &workspaceDir,
                                         const QString &oldProfileName,
                                         const QString &newProfileName);

    /**
     * @brief Clone catkin tools profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name to clone
     * @param newProfileName New profile name.
     * @return True if successful, otherwise false
     */
    static bool cloneCatkinToolsProfile(const Utils::FileName &workspaceDir,
                                        const QString &profileName, const QString &newProfileName);

    /**
     * @brief Create a new catkin tools profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return True if successful, otherwise false
     */
    static bool createCatkinToolsProfile(const Utils::FileName &workspaceDir,
                                         const QString profileName);

    /**
     * @brief Get the active catkin tools profile
     * @param workspaceDir Workspace directory path
     * @return True if successful, otherwise false
     */
    static QString getCatkinToolsActiveProfile(const Utils::FileName &workspaceDir);

    /**
     * @brief Set the catkin tools active profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return True if successful, otherwise false
     */
    static bool setCatkinToolsActiveProfile(const Utils::FileName &workspaceDir,
                                            const QString profileName);

    /**
     * @brief Get list of catkin tools profile names
     * @param workspaceDir Workspace directory path
     * @return QStringList of profile names
     */
    static QStringList getCatkinToolsProfileNames(const Utils::FileName &workspaceDir);

    /**
     * @brief Get file path to a catkin tools profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return Utils::FileName Path to profile
     */
    static Utils::FileName getCatkinToolsProfile(const Utils::FileName &workspaceDir,
                                                 const QString profileName);

    /**
     * @brief Get cmake build type argument
     * @param buildType Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
     * @return CMake build type argument
     */
    static QString getCMakeBuildTypeArgument(ROSUtils::BuildType &buildType);

    /**
     * @brief Get workspace source directory path
     * @param workspaceDir Workspace directory path
     * @param buildSystem Build system
     * @return Utils::FileName Path to source directory
     */
    static Utils::FileName getWorkspaceSourceSpace(const Utils::FileName &workspaceDir,
                                                   const BuildSystem buildSystem);

    /**
     * @brief Get workspace build directory path
     * @param workspaceDir Workspace directory path
     * @param buildSystem Build system
     * @return Utils::FileName Path to build directory
     */
    static Utils::FileName getWorkspaceBuildSpace(const Utils::FileName &workspaceDir,
                                                  const BuildSystem buildSystem);

    /**
     * @brief Get workspace devel directory path
     * @param workspaceDir Workspace directory path
     * @param buildSystem Build system
     * @return Utils::FileName Path to devel directory
     */
    static Utils::FileName getWorkspaceDevelSpace(const Utils::FileName &workspaceDir,
                                                  const BuildSystem buildSystem);

    /**
     * @brief Get workspace environment
     * @param workspaceDir Workspace directory path
     * @param rosDistribution ROS distribution (Hydro, Indigo, etc.)
     * @param buildSystem Build system
     * @return QProcessEnvironment
     */
    static QProcessEnvironment getWorkspaceEnvironment(const Utils::FileName &workspaceDir,
                                                       const QString &rosDistribution,
                                                       const BuildSystem buildSystem);

private:
    /**
     * @brief sourceWorkspaceHelper - Source workspace helper function
     * @param process - QProcess to execute source bash command
     * @param path - Path to workspace setup.bash
     * @return True if successful
     */
    static bool sourceWorkspaceHelper(QProcess *process, const QString &path);

    /**
     * @brief This will parse the CodeBlock file and get the build info (incudes, Cxx Flags, etc.)
     * @todo Need to figure out how to Cxx Flags.
     * @param workspaceDir Workspace directory path
     * @param buildSystem Workspace build system
     * @param package Package Info Objects
     * @return True if successful, otherwise false.
     */
    static bool getPackageBuildInfo(const Utils::FileName &workspaceDir,
                                    const BuildSystem buildSystem, ROSUtils::PackageInfo &package);

    /**
     * @brief Get path to the profiles directory
     * @param workspaceDir Workspace directory path
     * @return Utils::FileName Path to the profiles directory
     */
    static Utils::FileName getCatkinToolsProfilesPath(const Utils::FileName &workspaceDir);

    /**
     * @brief Get the file path for the catkin tools profiles yaml file
     *
     * This file contains the active profile.
     *
     * @param workspaceDir Workspace directory path
     * @return Utils::FileName Path to the profiles yaml file
     */
    static Utils::FileName getCatkinToolsProfilesYamlFile(const Utils::FileName &workspaceDir);

    /**
     * @brief Get the directory path to given profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return Utils::FileName Profile directory path
     */
    static Utils::FileName getCatkinToolsProfilePath(const Utils::FileName &workspaceDir,
                                                     const QString profileName);

    /**
     * @brief Get the file path to given profile's config.yaml file
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return Utils::FileName Path the profile's config.yaml file
     */
    static Utils::FileName getCatkinToolsProfileConfigFile(const Utils::FileName &workspaceDir,
                                                           const QString profileName);
};
} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSUTILS_H
