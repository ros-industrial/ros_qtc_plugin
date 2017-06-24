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
#include <utils/fileutils.h>
#include "ros_project_constants.h"

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
        BuildTypeMinSizeRel = 3,
        BuildTypeUserDefined = 4
    };

    enum TargetType {
        ExecutableType = 0,
        StaticLibraryType = 1,
        DynamicLibraryType = 2,
        UtilityType = 3
    };

    /** @brief The FolderContent struct used to store file and folder information */
    struct FolderContent {
        QStringList files;       /**< @brief Directory Files */
        QStringList directories; /**< @brief Directory Subdirectories */
    };

    /** @brief Contains relavent workspace information */
    struct WorkspaceInfo {
        Utils::FileName path;
        Utils::FileName sourcePath;
        Utils::FileName buildPath;
        Utils::FileName develPath;
        Utils::FileName installPath;
        Utils::FileName logPath;

        QString rosDistribution;
        BuildSystem buildSystem;
    };

    struct PackageTargetInfo {
        QString name;              /**< @brief Target name */
        TargetType type;           /**< @brief Target type */
        Utils::FileName flagsFile; /**< @brief Path to the Target's flags.cmake file */
        QStringList includes;      /**< @brief Target's include directories */
        QStringList flags;         /**< @brief Target's cxx build flags */
        QStringList defines;       /**< @brief Target's defines build flags */
    };
    typedef QList<PackageTargetInfo> PackageTargetInfoList;

    /** @brief Contains all relavent package information */
    struct PackageInfo {
        Utils::FileName path;           /**< @brief Package directory path */
        Utils::FileName filepath;       /**< @brief Package package.xml filepath */
        Utils::FileName buildFile;      /**< @brief Package CMakeLists.txt filepath */

        QString name;                   /**< @brief Package Name */
        QString version;                /**< @brief Package Version */
        QString description;            /**< @brief Package Description */
        QString maintainer;             /**< @brief Package Maintainer */
        QString license;                /**< @brief Package License */
        QString buildToolDepend;        /**< @brief Package Build Tool Dependency */
        QStringList buildDepends;       /**< @brief Package Build Dependencies */
        QStringList buildExportDepends; /**< @brief Package Build Export Dependencies */
        QStringList execDepends;        /**< @brief Package Execution Dependencies */
        QStringList testDepends;        /**< @brief Package Test Dependencies */
        QStringList docDepends;         /**< @brief Package Documentation Dependencies */
        bool metapackage;               /**< @brief Package is a metapackage if true */

        // Constructor
        PackageInfo() : metapackage(false) {}

        /**
         * @brief Check if package exists.
         * @return True if exists, otherwise false.
         */
        bool exists() const;
    };

    /** @brief Contains a packages relavent build informations */
    struct PackageBuildInfo {
        PackageBuildInfo(const PackageInfo packageInfo, const QStringList env)
        {
            parent = packageInfo;
            environment = env;
        }

        Utils::FileName path;          /**< @brief Path to the Package's build directory */
        Utils::FileName cbpFile;       /**< @brief Path to the Package's CodeBlocks file */
        QStringList environment;       /**< @brief Build Environment */
        PackageTargetInfoList targets; /**< @brief List of packages target's */
        PackageInfo parent;            /**< @brief Package information */

        /**
         * @brief Check if build information exists.
         * @return True if exists, otherwise false.
         */
        bool exists() const;
    };

    typedef QMap<QString, PackageBuildInfo> PackageBuildInfoMap;
    typedef QMap<QString, PackageInfo> PackageInfoMap;

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
     * @param workspaceInfo Workspace information
     * @return True if successful
     */
    static bool sourceWorkspace(QProcess *process,
                                const WorkspaceInfo &workspaceInfo);

    /**
     * @brief Check whether the provided workspace has been initialized
     * @param workspaceInfo Workspace information
     * @return True if workspace is initialized, otherwise false
     */
    static bool isWorkspaceInitialized(const WorkspaceInfo &WorkspaceInfo);

    /**
     * @brief Initialize workspace
     * @param process QProcess to execute the ROS bash command
     * @param workspaceInfo Workspace information
     * @return True if successfully executed, otherwise false
     */
    static bool initializeWorkspace(QProcess *process,
                                    const WorkspaceInfo &workspaceInfo);

    /**
     * @brief Initialize workspace folders
     * @param workspaceInfo Workspace information
     * @return True if successfully executed, otherwise false
     */
    static bool initializeWorkspaceFolders(const WorkspaceInfo &workspaceInfo);

    /**
     * @brief Build workspace
     * @param process QProcess to execute the catkin_make
     * @param workspaceInfo Workspace information
     * @return True if successfully executed, otherwise false
     */
    static bool buildWorkspace(QProcess *process,
                               const WorkspaceInfo &workspaceInfo);

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
     * @brief Get relevant workspace information
     * @param workspaceDir Path of the workspace
     * @param buildSystem Workspace build system
     * @return Workspace information
     */
    static WorkspaceInfo getWorkspaceInfo(const Utils::FileName &workspaceDir,
                                          const BuildSystem &buildSystem,
                                          const QString &rosDistribution);

    /**
     * @brief Get all of the workspace packages and its neccessary information.
     * @param workspaceInfo Workspace information
     * @param cachedPackageInfo Cached Package information to use if it fails
     * @return QMap(Package Name, PackageInfo)
     */
    static PackageInfoMap getWorkspacePackageInfo(const WorkspaceInfo &workspaceInfo,
                                                  const PackageInfoMap *cachedPackageInfo = NULL);

    /**
     * @brief Get a packages build information
     * @param workspaceInfo Workspace information
     * @param packageInfo Package Information
     * @param cachedPackageBuildInfo Cached Package build information if it fails
     * @return PackageBuildInfo
     */
    static PackageBuildInfoMap getWorkspacePackageBuildInfo(const WorkspaceInfo &workspaceInfo,
                                                            const PackageInfoMap &packageInfo,
                                                            const PackageBuildInfoMap *cachedPackageBuildInfo = NULL);

    /**
     * @brief Executes the bash command "rospack list" and returns a map of QMap(Package Name, Path
     * to package)
     * @param env Is the environment to use for getting the list of available packages.
     * @return QMap(Package Name, Path to package)
     */
    static QMap<QString, QString> getROSPackages(const QStringList &env);

    /**
     * @brief Get the path to every package in the workspace.
     * @param workspaceInfo Workspace information
     * @return QMap(Package Name, Path to package)
     */
    static QMap<QString, QString> getWorkspacePackagePaths(const WorkspaceInfo &workspaceInfo);

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
                                         const QString &profileName);

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
                                        const QString &profileName,
                                        const QString &newProfileName);

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
     * @brief Set default catkin tools profile
     * @param workspaceDir Workspace directory path
     * @return True if successful, otherwise false
     */
    static QString setCatkinToolsDefaultProfile(const Utils::FileName &workspaceDir);

    /**
     * @brief Set the catkin tools active profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return True if successful, otherwise false
     */
    static bool setCatkinToolsActiveProfile(const Utils::FileName &workspaceDir,
                                            const QString &profileName);

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
                                                 const QString &profileName);

    /**
     * @brief Get cmake build type argument
     * @param buildType Build type (Debug, Release, RelWithDebInfo, MinSizeRel)
     * @return CMake build type argument
     */
    static QString getCMakeBuildTypeArgument(ROSUtils::BuildType &buildType);

    /**
     * @brief Get workspace environment
     * @param workspaceInfo Workspace information
     * @param rosDistribution ROS distribution (Hydro, Indigo, etc.)
     * @return QProcessEnvironment
     */
    static QProcessEnvironment getWorkspaceEnvironment(const WorkspaceInfo &workspaceInfo);

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
     * @param workspaceInfo Workspace information
     * @param package Package Info Objects
     * @return True if successful, otherwise false.
     */
    static bool parseCodeBlocksFile(const WorkspaceInfo &workspaceInfo,
                                    PackageBuildInfo &package);

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
     * @brief Set curent profile for the catkin tools profiles yaml file
     *
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return True if successful, otherwise false.
     */
    static bool setCatkinToolsProfilesYamlFile(const Utils::FileName &workspaceDir,
                                               const QString &profileName);
    /**
     * @brief Get the directory path to given profile
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return Utils::FileName Profile directory path
     */
    static Utils::FileName getCatkinToolsProfilePath(const Utils::FileName &workspaceDir,
                                                     const QString &profileName);

    /**
     * @brief Get the file path to given profile's config.yaml file
     * @param workspaceDir Workspace directory path
     * @param profileName Profile name
     * @return Utils::FileName Path the profile's config.yaml file
     */
    static Utils::FileName getCatkinToolsProfileConfigFile(const Utils::FileName &workspaceDir,
                                                           const QString &profileName);

    /**
     * @brief Find a given packages build directory
     * @param workspaceInfo Workspace information
     * @param packageInfo Package Information
     * @return Packages build directory
     */
    static bool findPackageBuildDirectory(const WorkspaceInfo &workspaceInfo,
                                          const PackageInfo &packageInfo,
                                          Utils::FileName &packageBuildPath);
};
} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSUTILS_H
