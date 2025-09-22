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
#include "ros_utils.h"
#include "ros_project_constants.h"
#include "ros_packagexml_parser.h"
#include "ros_settings_page.h"
#include "ros_project_plugin.h"

#include <utils/fileutils.h>
#include <coreplugin/messagemanager.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <regex>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace ROSProjectManager {
namespace Internal {

ROSUtils::ROSUtils()
{

}

QString ROSUtils::buildTypeName(const ROSUtils::BuildType buildType)
{
    switch (buildType) {
    case ROSUtils::BuildTypeDebug:
        return QStringLiteral("Debug");
    case ROSUtils::BuildTypeMinSizeRel:
        return QStringLiteral("Minimum Size Release");
    case ROSUtils::BuildTypeRelWithDebInfo:
        return QStringLiteral("Release with Debug Information");
    case ROSUtils::BuildTypeRelease:
        return QStringLiteral("Release");
    default:
        return QStringLiteral("User Defined");
    }
}

bool ROSUtils::sourceROS(QProcessEnvironment &env, const Utils::FilePath &rosDistribution)
{
  sourceWorkspaceHelper(env, Utils::FilePath(rosDistribution).pathAppended(Constants::ROS_SOURCE_FILE_BASH).toFSPathString());
  return true;
}

bool ROSUtils::sourceWorkspace(QProcessEnvironment &env, const WorkspaceInfo &workspaceInfo)
{
    if (!initializeWorkspace(env, workspaceInfo))
        return false;

    Utils::FilePath sourcePath(workspaceInfo.develPath);
    if (workspaceInfo.install)
      sourcePath = Utils::FilePath(workspaceInfo.installPath);

    const Utils::FilePath source_bash_file = sourcePath.pathAppended(Constants::ROS_SOURCE_FILE_BASH);
    const Utils::FilePath source_shell_file = sourcePath.pathAppended(Constants::ROS_SOURCE_FILE_SHELL);
    QString source_path;
    if (source_bash_file.exists())
    {
        Core::MessageManager::writeSilently(QObject::tr("[ROS Debug] Sourced workspace: %1.").arg(source_bash_file.toFSPathString()));
        source_path = source_bash_file.toFSPathString();
    }
    else if (source_shell_file.exists())
    {
        // Some reason if a workspace does not contain at least one catkin package it does not generate a setup.bash only a setup.sh
        Core::MessageManager::writeSilently(QObject::tr("[ROS Debug] Sourced workspace: %1.").arg(source_shell_file.toFSPathString()));
        source_path = source_shell_file.toFSPathString();
    }
    else
    {
        Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to source workspace because either of these files do not exist: %1 or %2.").arg(source_bash_file.toFSPathString(), source_shell_file.toFSPathString()));
        source_path = QString{};
    }

    sourceWorkspaceHelper(env, source_path);
    return true;
}

bool ROSUtils::isWorkspaceInitialized(const WorkspaceInfo &workspaceInfo)
{
    switch (workspaceInfo.buildSystem) {
    case ROSUtils::CatkinMake:
    {
        Utils::FilePath topCMake(workspaceInfo.sourcePath);
        topCMake = topCMake.pathAppended(QStringLiteral("CMakeLists.txt"));
        Utils::FilePath catkin_workspace(workspaceInfo.path);
        catkin_workspace = catkin_workspace.pathAppended(QStringLiteral(".catkin_workspace"));

        if (topCMake.exists() && catkin_workspace.exists() && workspaceInfo.sourcePath.exists())
          return true;

        return false;
    }
    case ROSUtils::CatkinTools:
    {
        Utils::FilePath catkin_tools(workspaceInfo.path);
        catkin_tools = catkin_tools.pathAppended(QStringLiteral(".catkin_tools"));
        if (catkin_tools.exists() && workspaceInfo.sourcePath.exists())
          return true;

        return false;
    }
    case ROSUtils::Colcon:
    {
        if (workspaceInfo.sourcePath.exists())
          return true;

        return false;
    }
    }

    return false;
}

bool ROSUtils::initializeWorkspaceFolders(const WorkspaceInfo &workspaceInfo)
{
    if (!workspaceInfo.sourcePath.exists())
        if( ! QDir().mkpath(workspaceInfo.sourcePath.toFSPathString()) ) {
            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to initialize workspace folder: %1.").arg(workspaceInfo.sourcePath.toFSPathString()));
            return false;
        }

    if (!workspaceInfo.logPath.exists())
        if( ! QDir().mkpath(workspaceInfo.logPath.toFSPathString()) ) {
            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to initialize workspace folder: %1.").arg(workspaceInfo.logPath.toFSPathString()));
            return false;
        }

    if (!workspaceInfo.buildPath.exists())
        if( ! QDir().mkpath(workspaceInfo.buildPath.toFSPathString()) ) {
            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to initialize workspace folder: %1.").arg(workspaceInfo.buildPath.toFSPathString()));
            return false;
        }

    if (!workspaceInfo.develPath.exists())
        if( ! QDir().mkpath(workspaceInfo.develPath.toFSPathString()) ) {
            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to initialize workspace folder: %1.").arg(workspaceInfo.develPath.toFSPathString()));
            return false;
        }

    if (!workspaceInfo.installPath.exists())
        if( ! QDir().mkpath(workspaceInfo.installPath.toFSPathString()) ) {
            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to initialize workspace folder: %1.").arg(workspaceInfo.installPath.toFSPathString()));
            return false;
        }

    return true;
}

bool ROSUtils::initializeWorkspace(QProcessEnvironment &env, const WorkspaceInfo &workspaceInfo)
{
    WorkspaceInfo workspace = workspaceInfo;

    if (sourceROS(env, workspaceInfo.rosDistribution)) {
        if (!isWorkspaceInitialized(workspaceInfo))
        {
            QProcess process;
            process.setProcessEnvironment(env);

            switch (workspaceInfo.buildSystem) {
            case CatkinMake:
            {
                if( !initializeWorkspaceFolders(workspaceInfo) )
                    return false;

                process.setWorkingDirectory(workspaceInfo.sourcePath.toFSPathString());
                process.start(QStringLiteral("bash"), QStringList() << QStringList() << QStringLiteral("-c") << QStringLiteral("catkin_init_workspace"));

                if( !process.waitForFinished() )
                    return false;

                break;
            }
            case CatkinTools:
            {
                setCatkinToolsDefaultProfile(workspace.path);

                workspace = ROSUtils::getWorkspaceInfo(workspace.path,
                                                       workspace.buildSystem,
                                                       workspace.rosDistribution);

                if( !initializeWorkspaceFolders(workspace) )
                    return false;

                process.setWorkingDirectory(workspace.path.toFSPathString());
                process.start(QStringLiteral("bash"), QStringList() << QStringLiteral("-c") << QStringLiteral("catkin init"));

                if( !process.waitForFinished() )
                    return false;

                break;
            }
            case Colcon:
            {
                workspace = ROSUtils::getWorkspaceInfo(workspace.path,
                                                       workspace.buildSystem,
                                                       workspace.rosDistribution);

                if( !initializeWorkspaceFolders(workspace) )
                    return false;

                break;
            }
            } // switch

            if (process.exitStatus() != QProcess::CrashExit)
                return buildWorkspace(process, workspace);

            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to initialize workspace: %1.").arg(workspace.path.toFSPathString()));
            return false;
        } // if
    }

    return true;
}

bool ROSUtils::buildWorkspace(QProcess &process, const WorkspaceInfo &workspaceInfo)
{
    switch(workspaceInfo.buildSystem) {
    case CatkinMake:
    {
        process.setWorkingDirectory(workspaceInfo.path.toFSPathString());
        process.start(QStringLiteral("bash"), QStringList() << QStringLiteral("-c") << QStringLiteral("catkin_make --cmake-args -G \"CodeBlocks - Unix Makefiles\""));
        process.waitForFinished();
        break;
    }
    case CatkinTools:
    {
        process.setWorkingDirectory(workspaceInfo.path.toFSPathString());
        process.start(QStringLiteral("bash"), QStringList() << QStringLiteral("-c") << QStringLiteral("catkin build --cmake-args -G \"CodeBlocks - Unix Makefiles\""));
        process.waitForFinished();
        break;
    }
    case Colcon:
    {
        process.setWorkingDirectory(workspaceInfo.path.toFSPathString());
        process.start(QStringLiteral("bash"), QStringList() << QStringLiteral("-c") << QStringLiteral("colcon build"));
        process.waitForFinished();
        break;
    }
    }

    if (process.exitStatus() != QProcess::CrashExit)
        return true;

    Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Failed to build workspace: %1.").arg(workspaceInfo.path.toFSPathString()));
    return false;
}

const QList<Utils::FilePath> ROSUtils::installedDistributions()
{
  QSharedPointer<ROSSettings> ros_settings = ROSProjectPlugin::instance()->settings();
  const Utils::FilePath custom_ros_path = ros_settings->custom_dist_path;
  QList<Utils::FilePath> distributions;
  if(custom_ros_path.exists())
  {
    const Utils::FilePaths entries = custom_ros_path.dirEntries(QDir::NoDotAndDotDot | QDir::Dirs);
    for (const Utils::FilePath &entry : entries)
    {
      if ((entry / Constants::ROS_SOURCE_FILE).exists())
      {
        distributions.append(custom_ros_path);
      }
    }
  }

  const Utils::FilePath default_ros_path = ros_settings->default_dist_path;
  if (default_ros_path.exists())
  {
    const Utils::FilePaths entries = default_ros_path.dirEntries(QDir::NoDotAndDotDot | QDir::Dirs);
    for (const Utils::FilePath &entry : entries)
    {
      if ((entry / Constants::ROS_SOURCE_FILE).exists())
      {
        distributions.append(entry);
      }
    }
  }

  if (distributions.isEmpty())
      Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] ROS does not appear to be installed.\n Check ROS Settings page to verify that the install location is valid."));

  return distributions;
}

void ROSUtils::sourceWorkspaceHelper(QProcessEnvironment &env, const QString &path)
{
    if (path.isEmpty())
        return;

    QProcess process;

    const QString cmd = QStringLiteral("source ") + path + QStringLiteral(" && env");
    process.start(QStringLiteral("bash"), QStringList());
    process.waitForStarted();
    process.write(cmd.toLatin1());
    process.closeWriteChannel();
    process.waitForFinished();

    if (process.exitStatus() == QProcess::CrashExit)
        return;

    while (process.canReadLine()) {
        const QStringList env_kv = QString::fromLocal8Bit(process.readLine().trimmed()).split('=');
        if (env_kv.size() == 2) {
            env.insert(env_kv[0], env_kv[1]);
        }
    }
}

bool ROSUtils::generateQtCreatorWorkspaceFile(QXmlStreamWriter &xmlFile, const ROSProjectFileContent &content)
{
    xmlFile.setAutoFormatting(true);
    xmlFile.writeStartDocument();
    xmlFile.writeStartElement(QStringLiteral("Workspace"));

    if (!content.distribution.path().trimmed().isEmpty())
    {
        xmlFile.writeStartElement(QStringLiteral("Distribution"));
        xmlFile.writeAttribute(QStringLiteral("path"), content.distribution.toFSPathString());
        xmlFile.writeEndElement();
    }

    xmlFile.writeStartElement(QStringLiteral("DefaultBuildSystem"));
    xmlFile.writeAttribute(QStringLiteral("value"), QString::number(content.defaultBuildSystem));
    xmlFile.writeEndElement();

    xmlFile.writeEndElement();
    xmlFile.writeEndDocument();
    return xmlFile.hasError();
}

bool ROSUtils::parseQtCreatorWorkspaceFile(const Utils::FilePath &filePath, ROSProjectFileContent &content)
{
    QXmlStreamReader workspaceXml;
    QFile workspaceFile(filePath.toFSPathString());
    if (workspaceFile.open(QFile::ReadOnly | QFile::Text))
    {
        workspaceXml.setDevice(&workspaceFile);
        while(workspaceXml.readNextStartElement())
        {
            if (workspaceXml.name() == QStringLiteral("Distribution"))
            {
                QList<Utils::FilePath> distributions = ROSUtils::installedDistributions();
                QXmlStreamAttributes attributes = workspaceXml.attributes();
                if (attributes.hasAttribute(QStringLiteral("path")))
                {
                    content.distribution = Utils::FilePath::fromString(attributes.value(QStringLiteral("path")).toString());
                    if (!distributions.empty() && !distributions.contains(content.distribution))
                    {
                        Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Project file distribution [%1] is not installed. Setting to [%2], if incorrect modify project file [%3].").arg(content.distribution.toFSPathString(), distributions.first().toFSPathString(), filePath.fileName()));
                        content.distribution = distributions.first();
                    }
                }
                else
                {
                    if (!distributions.isEmpty())
                    {
                        content.distribution = distributions.first();
                        Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Unable to find ROS distributions."));
                    }
                    else
                    {
                        Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Project file Distribution tag did not have a name attribute."));
                    }
                }

                workspaceXml.readNextStartElement();
            }
            else if (workspaceXml.name() == QStringLiteral("DefaultBuildSystem"))
            {
                QXmlStreamAttributes attributes = workspaceXml.attributes();
                if (attributes.hasAttribute(QStringLiteral("value")))
                {
                    content.defaultBuildSystem = (ROSUtils::BuildSystem)attributes.value(QStringLiteral("value")).toInt();
                }
                else
                {
                    content.defaultBuildSystem = ROSUtils::Colcon;
                    Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Project file DefaultBuildSystem tag did not have a value attribute."));
                }

                workspaceXml.readNextStartElement();
            }
        }

        return true;
    }

    Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Error opening Workspace Project File: %1.").arg(filePath.toFSPathString()));
    return false;
}

void ROSUtils::getDefaultFolderContentFilters(QStringList& folderNameFilters, QStringList& fileNameFilters)
{
  folderNameFilters.push_back("\\.git");
  fileNameFilters.push_back("^.*\\.autosave");
}

ROSUtils::FolderContent ROSUtils::getFolderContent(const QString &folder, const QStringList& folderNameFilters, const QStringList& fileNameFilters)
{
  ROSUtils::FolderContent content;

  // Get Directory data
  content.directories = QDir(folder).entryList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Hidden);
  content.removeDirectories(folderNameFilters);

  content.files = QDir(folder).entryList(QDir::NoDotAndDotDot | QDir::Files | QDir::Hidden);
  content.removeFiles(fileNameFilters);

  return content;
}

QHash<QString, ROSUtils::FolderContent> ROSUtils::getFolderContentRecursive(const Utils::FilePath &folderPath, QStringList &fileList, QStringList& directoryList)
{
    QHash<QString, ROSUtils::FolderContent> workspaceFiles;

    QString folder = folderPath.toFSPathString();

    // Need to remove unwanted directories
    QStringList folderNameFilters, fileNameFilters;
    getDefaultFolderContentFilters(folderNameFilters, fileNameFilters);

    const ROSUtils::FolderContent content = getFolderContent(folder, folderNameFilters, fileNameFilters);

    workspaceFiles[folder] = content;

    for (const QString& file : content.files)
        fileList.append(QDir(folder).absoluteFilePath(file));

    for (const QString& directory : content.directories)
        directoryList.append(QDir(folder).absoluteFilePath(directory));

    // Get SubDirectory Information
    const QDir subDir(folder);
    QDirIterator itSrc(subDir.absolutePath(), QDir::NoDotAndDotDot | QDir::Dirs | QDir::Hidden, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    QList<QString> excludeDir;
    while (itSrc.hasNext())
    {
        folder = itSrc.next();

        QString folder_name = Utils::FilePath::fromString(folder).fileName();
        bool found = false;
        for (const auto& filter : std::as_const(folderNameFilters))
        {
          QRegularExpression rx(filter);
          if (rx.match(folder_name).hasMatch())
          {
            excludeDir.push_back(folder + QLatin1Char('/'));
            found = true;
            break;
          }
        }

        if (found)
          continue;

        bool skip = false;
        for (const QString& exclude : excludeDir)
        {
            if (folder.startsWith(exclude))
            {
                skip = true;
                break;
            }
        }

        if (skip) continue;

        const ROSUtils::FolderContent content = getFolderContent(folder, folderNameFilters, fileNameFilters);

        workspaceFiles[folder] = content;

        for (const QString& file : content.files)
            fileList.append(QDir(folder).absoluteFilePath(file));

        for (const QString& directory : content.directories)
            directoryList.append(QDir(folder).absoluteFilePath(directory));
    }

    return workspaceFiles;
}

ROSUtils::PackageInfoMap ROSUtils::getWorkspacePackageInfo(const WorkspaceInfo &workspaceInfo, const PackageInfoMap *cachedPackageInfo)
{
    PackageInfoMap wsPackageInfo;
    QMap<QString, QString> packages =  ROSUtils::getWorkspacePackagePaths(workspaceInfo);

    for(const auto& it : std::as_const(packages))
    {
        Utils::FilePath pkgXml = Utils::FilePath::fromString(it).pathAppended("package.xml");
        ROSUtils::PackageInfo packageInfo;
        ROSPackageXmlParser pkgParser;
        if (pkgParser.parsePackageXml(pkgXml, packageInfo))
        {
            if (packageInfo.metapackage)
                continue;

            wsPackageInfo.insert(packageInfo.name, packageInfo);
            continue;
        }

        // Check if there is cached build info available
        if (cachedPackageInfo)
        {
            auto packIt = cachedPackageInfo->find(packageInfo.name);
            if (packIt != cachedPackageInfo->end())
            {
                Core::MessageManager::writeSilently(QObject::tr("[ROS Info] Using cached package information for package: %1.").arg(packageInfo.name));
                wsPackageInfo.insert(packIt.value().name, packIt.value());
            }
        }
    }
    return wsPackageInfo;
}

ROSUtils::PackageBuildInfoMap ROSUtils::getWorkspacePackageBuildInfo(const WorkspaceInfo &workspaceInfo,
                                                                     const PackageInfoMap &packageInfo,
                                                                     const PackageBuildInfoMap *cachedPackageBuildInfo)
{
    PackageBuildInfoMap wsBuildInfo;
    for (const PackageInfo& package : packageInfo)
    {
        PackageBuildInfo buildInfo(package);
        if (findPackageBuildDirectory(workspaceInfo, package, buildInfo.path))
        {
            // Get package's code block file
            buildInfo.cbpFile = buildInfo.path.pathAppended(QString("%1.cbp").arg(package.name));
            buildInfo.apiReplyPath = buildInfo.path / ".cmake" / "api" / "v1" / "reply";

            // If does not exist for default Project.cbp file
            if (!buildInfo.cbpFile.exists())
            {
              Utils::FilePath temp = buildInfo.path.pathAppended("Project.cbp");
              if (temp.exists())
                  buildInfo.cbpFile = temp;
            }

            if (buildInfo.apiReplyPath.isDir())
            {
                if (ROSUtils::parseCMakeFileAPI(buildInfo))
                {
                    wsBuildInfo.insert(package.name, buildInfo);
                    continue;
                }
                else
                {
                    Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Unable to parse build information for package: %1.").arg(package.name));
                }
            }
            else if (buildInfo.cbpFile.exists())
            {
                Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] CodeBlocks project file parsing is deprecated and will be remove in future! (package: %1)").arg(package.name));
                if (ROSUtils::parseCodeBlocksFile(workspaceInfo, buildInfo))
                {
                    wsBuildInfo.insert(package.name, buildInfo);
                    continue;
                }
                else
                {
                    Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Unable to parse build information for package: %1.").arg(package.name));
                }
            }
            else
            {
                Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Unable to locate package %1 build file: %2.").arg(package.name, buildInfo.cbpFile.toFSPathString()));
            }
        }
        else
        {
            Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Unable to locate build directory for package: %1.").arg(package.name));
        }

        // Check if there is cached build info available
        if (cachedPackageBuildInfo)
        {
            auto packIt = cachedPackageBuildInfo->find(package.name);
            if (packIt != cachedPackageBuildInfo->end())
            {
                Core::MessageManager::writeSilently(QObject::tr("[ROS Info] Using cached package build information for package: %1.").arg(package.name));
                wsBuildInfo.insert(package.name, packIt.value());
            }
        }

    }

    return wsBuildInfo;
}

bool ROSUtils::parseCodeBlocksFile(const WorkspaceInfo &workspaceInfo, ROSUtils::PackageBuildInfo &buildInfo)
{
  QMap<QString, PackageTargetInfoPtr> targetMap;

  // Parse CodeBlocks Project File
  // Need to search for all of the tags <Add directory="include path" />
  QXmlStreamReader cbpXml;

  QFile cbpFile(buildInfo.cbpFile.toFSPathString());
  if (!cbpFile.open(QFile::ReadOnly | QFile::Text))
  {
    Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Error opening CodeBlocks Project File: %1.").arg(buildInfo.cbpFile.toFSPathString()));
    return false;
  }

  // make sure targets are cleared
  buildInfo.targets.clear();

  // build time include directory
  Utils::FilePath buildtimeInclude(workspaceInfo.develPath);
  if (workspaceInfo.install)
    buildtimeInclude = Utils::FilePath(workspaceInfo.installPath);

  buildtimeInclude = buildtimeInclude.pathAppended(QStringLiteral("include"));

  cbpXml.setDevice(&cbpFile);
  cbpXml.readNext();
  while(!cbpXml.atEnd())
  {
    if(cbpXml.isStartElement())
    {
      if(cbpXml.name() == QStringLiteral("Target"))
      {
        QString targetName;
        QString targetWorkingDir = buildInfo.path.toFSPathString();
        Utils::FilePaths targetLocalIncludes;
        Utils::FilePaths targetSystemIncludes;
        TargetType targetType = UtilityType;
        if (cbpXml.attributes().hasAttribute("title"))
        {
          QString title =cbpXml.attributes().value("title").toString();
          if (!title.endsWith(QStringLiteral("/fast")) && !title.endsWith(QStringLiteral("_automoc")) && !title.startsWith(QStringLiteral("gtest")))
          {
            targetName = title;
          }
          else
          {
            cbpXml.readNext();
            continue;
          }
        }
        else
        {
          cbpXml.readNext();
          continue;
        }

        cbpXml.readNext();
        while (cbpXml.name() != QStringLiteral("Target"))
        {
            if(cbpXml.isStartElement())
            {
                if(cbpXml.name() == QStringLiteral("Option"))
                {
                    if (cbpXml.attributes().hasAttribute("type"))
                    {
                        QString attribute_value = cbpXml.attributes().value("type").toString();
                        if (attribute_value == "0" || attribute_value == "1")
                            targetType = ExecutableType;
                        else if (attribute_value == "2")
                            targetType = StaticLibraryType;
                        else if (attribute_value == "3")
                            targetType = DynamicLibraryType;
                        else
                            targetType = UtilityType;
                    }

                    if (cbpXml.attributes().hasAttribute("working_dir"))
                    {
                        targetWorkingDir = cbpXml.attributes().value("working_dir").toString();
                    }
                }

                if(cbpXml.name() == QStringLiteral("Add"))
                {
                    if (cbpXml.attributes().hasAttribute("directory"))
                    {
                        Utils::FilePath attribute_value = Utils::FilePath::fromString(cbpXml.attributes().value("directory").toString());
                        if (attribute_value.toFSPathString().startsWith(workspaceInfo.path.toFSPathString()))
                        {
                            if (!targetLocalIncludes.contains(attribute_value))
                              targetLocalIncludes.append(attribute_value);
                        }
                        else if(!targetSystemIncludes.contains(attribute_value))
                        {
                            targetSystemIncludes.append(attribute_value);
                        }
                    }
                }
            }
            cbpXml.readNext();
        }

        // Only need to add target types ExecutableType and StaticLibraryType to the code model
        if (targetType != UtilityType)
        {
            targetLocalIncludes.append(buildtimeInclude);

            PackageTargetInfoPtr targetInfo = std::make_shared<PackageTargetInfo>();
            targetInfo->name = targetName;
            targetInfo->type = targetType;
            targetInfo->flagsFile = Utils::FilePath::fromString(targetWorkingDir).pathAppended("CMakeFiles").pathAppended(QString("%1.dir").arg(targetName)).pathAppended("flags.make");
            if (!targetInfo->flagsFile.exists())
            {
                QDirIterator it(buildInfo.path.toFSPathString(), QStringList() << QString("%1.dir").arg(targetName), QDir::NoFilter, QDirIterator::Subdirectories);
                while (it.hasNext())
                {
                    Utils::FilePath found_path = Utils::FilePath::fromString(it.next()).pathAppended("flags.make");
                    if (found_path.exists())
                    {
                      targetInfo->flagsFile = found_path;
                      break;
                    }
                }
            }

            // The order matters so it will order local first then system
            targetInfo->includes = targetLocalIncludes;
            targetInfo->includes.append(targetSystemIncludes);

            buildInfo.targets.append(targetInfo);
            targetMap[targetName] = targetInfo;
        }
      }
      else if(cbpXml.name() == QStringLiteral("Unit"))
      {
        QString filename;
        if (cbpXml.attributes().hasAttribute("filename"))
        {
            filename = cbpXml.attributes().value("filename").toString();
        }
        cbpXml.readNext();
        while (cbpXml.name() != QStringLiteral("Unit"))
        {
            if(cbpXml.isStartElement())
            {
                if(cbpXml.name() == QStringLiteral("Option"))
                {
                    if (cbpXml.attributes().hasAttribute("target"))
                    {
                        QString temp = cbpXml.attributes().value("target").toString();
                        auto it = targetMap.find(temp);
                        if (it != targetMap.end())
                          it.value()->source_files.append(Utils::FilePath::fromString(filename));
                    }
                }
            }
            cbpXml.readNext();
        }
      }
    }
    cbpXml.readNext();
  }

//  Next search the package directory for any missed include folders
//  QString includePath;
//  QDirIterator itPackage(package.path, QStringList() << QStringLiteral("include"), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
//  while (itPackage.hasNext())
//  {
//    includePath = itPackage.next();
//    if(!workspace_includes.contains(includePath))
//      workspace_includes.append(includePath);
//  }

  for (const PackageTargetInfoPtr &it : std::as_const(buildInfo.targets))
  {
      // Next need to parse flags.cmake for flags and defines
      if (it->flagsFile.exists())
      {
          QFile flagsFile(it->flagsFile.toFSPathString());
          if (!flagsFile.open(QFile::ReadOnly | QFile::Text))
          {
            Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Error opening flags file: %1.").arg(it->flagsFile.toFSPathString()));
            it->flags.append(QStringLiteral("-std=c++11"));
            continue;
          }

          QTextStream flagsStream(&flagsFile);
          while (!flagsStream.atEnd()) {
              QString line = flagsStream.readLine().trimmed();
              if (line.startsWith("CXX_FLAGS ="))
                  it->flags = line.mid(11).trimmed().split(' ', Qt::SkipEmptyParts);

              QStringList defines;
              if (line.startsWith("CXX_DEFINES ="))
                  defines = line.mid(13).trimmed().split(' ', Qt::SkipEmptyParts);

              // Need to remove -D from the define and escaped quotes
              for (auto& d : defines)
                  it->defines.push_back(d.trimmed().mid(2).remove('\\'));
          }
      }
      else
      {
          Core::MessageManager::writeSilently(QObject::tr("[ROS Warning] Flags file does not exist: %1.").arg(it->flagsFile.toFSPathString()));
          it->flags.append(QStringLiteral("-std=c++11"));
      }
  }

  return true;
}

bool ROSUtils::parseCMakeFileAPI(PackageBuildInfo &package)
{
    // find "index-*.json"
    const std::regex pattern(R"(index-.*\.json)");
    const Utils::FilePaths files = package.apiReplyPath.dirEntries(QDir::Filter::Files);
    const Utils::FilePaths::ConstIterator it = std::find_if(files.cbegin(), files.cend(), [&pattern](const Utils::FilePath &fp){ return std::regex_match(fp.fileName().toStdString(), pattern); });
    if (it == files.cend()) {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to locate index file for package: '%1'").arg(package.parent.name));
        return false;
    }

    // parse index
    QFile index_file(it->toFSPathString());
    if (!index_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to read index file: '%1'").arg(index_file.errorString()));
        return false;
    }
    const QByteArray index_json_data = index_file.readAll();
    index_file.close();

    const QJsonDocument doc_index = QJsonDocument::fromJson(index_json_data);
    if (doc_index.isNull() || !doc_index.isObject()) {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to parse index file"));
        return false;
    }

    const QJsonObject obj_index = doc_index.object();

    if (!obj_index.contains("objects") || !obj_index["objects"].isArray())
        return false;

    // find the codemodel filename
    const QJsonArray &index_objs = obj_index["objects"].toArray();
    QString codemodel_filename;
    for (const QJsonValue &val : index_objs) {
        if (val.isObject() &&
            val.toObject().contains("kind") && val["kind"].isString() && val["kind"] == "codemodel" &&
            val.toObject().contains("jsonFile") && val["jsonFile"].isString()) {
            codemodel_filename = val["jsonFile"].toString();
            break;
        }
    }

    if (codemodel_filename.toStdString().empty())
        return false;

    const Utils::FilePath codemodel_filepath = package.apiReplyPath / codemodel_filename;

    // parse codemodel
    QFile codemodel_file(codemodel_filepath.toFSPathString());
    if (!codemodel_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to read codemodel file: '%1'").arg(codemodel_file.errorString()));
        return false;
    }
    const QByteArray codemodel_json_data = codemodel_file.readAll();
    codemodel_file.close();

    const QJsonDocument doc_codemodel = QJsonDocument::fromJson(codemodel_json_data);
    if (doc_codemodel.isNull() || !doc_codemodel.isObject()) {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to parse codemodel file"));
        return false;
    }

    const QJsonObject obj_codemodel = doc_codemodel.object();

    if (!(obj_codemodel.contains("paths") && obj_codemodel["paths"].isObject() &&
          obj_codemodel["paths"].toObject().contains("source")))
        return false;

    const Utils::FilePath source_toplevel_path = \
        Utils::FilePath::fromString(obj_codemodel["paths"].toObject()["source"].toString());

    if (!obj_codemodel.contains("configurations") || !obj_codemodel["configurations"].isArray())
        return false;

    const QJsonArray &cm_cfgs = obj_codemodel["configurations"].toArray();
    QJsonArray targets;
    for (const QJsonValue &val : cm_cfgs) {
        if (val.isObject() && val.toObject().contains("targets") && val["targets"].isArray()) {
            targets = val["targets"].toArray();
            break;
        }
    }

    if (targets.isEmpty()) {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Package '%1' has no targets").arg(package.parent.name));
        return false;
    }

    std::unordered_map<QString, QString> target_filenames;

    for (const QJsonValue &val : std::as_const(targets)) {
        if (!(val.toObject().contains("name") && val["name"].isString() &&
              val.toObject().contains("jsonFile") && val["jsonFile"].isString()))
            continue;
        target_filenames[val["name"].toString()] = val["jsonFile"].toString();
    }

    // parse target files
    for (const auto &[target_name, filename] : target_filenames) {
        QFile target_file((package.apiReplyPath / filename).toFSPathString());
        if (!target_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to read target file: '%1'").arg(target_file.errorString()));
            return false;
        }
        const QByteArray target_json_data = target_file.readAll();
        target_file.close();

        const QJsonDocument doc_target = QJsonDocument::fromJson(target_json_data);
        if (doc_target.isNull() || !doc_target.isObject()) {
            Core::MessageManager::writeFlashing(QObject::tr("[ROS Warning] Unable to parse target file"));
            return false;
        }

        const QJsonObject obj_target = doc_target.object();

        if (!(obj_target.contains("type") && obj_target["type"].isString()))
            continue;

        PackageTargetInfoPtr targetInfo = std::make_shared<PackageTargetInfo>();

        // only consider ELF files
        const QString type = obj_target["type"].toString();
        if (type == QString("EXECUTABLE"))
            targetInfo->type = ROSUtils::TargetType::ExecutableType;
        else if (type == QString("STATIC_LIBRARY"))
            targetInfo->type = ROSUtils::TargetType::StaticLibraryType;
        else if (type == QString("SHARED_LIBRARY"))
            targetInfo->type = ROSUtils::TargetType::DynamicLibraryType;
        else if (type == QString("UTILITY"))
            targetInfo->type = ROSUtils::TargetType::UtilityType;
        else
            continue;

        if (!(obj_target.contains("name") && obj_target["name"].isString()))
            continue;

        targetInfo->name = obj_target["name"].toString();

        // source files
        if (!(obj_target.contains("sources") && obj_target["sources"].isArray()))
            continue;

        const QJsonArray &target_sources = obj_target["sources"].toArray();
        for (const QJsonValue &val : std::as_const(target_sources)) {
            if (!val["path"].isString())
                continue;
            const Utils::FilePath source_path = Utils::FilePath::fromString(val["path"].toString());
            if(source_path.isAbsolutePath())
                targetInfo->source_files.append(source_path);
            else
                targetInfo->source_files.append(source_toplevel_path / source_path.toFSPathString());
        }

        // compile settings
        if (!(obj_target.contains("compileGroups") && obj_target["compileGroups"].isArray() &&
              obj_target["compileGroups"].toArray().size() != 0 && obj_target["compileGroups"].toArray()[0].isObject()))
            continue;

        const QJsonArray &cgs = obj_target["compileGroups"].toArray();
        for (const QJsonValue &val_cg : std::as_const(cgs)) {
            if(!val_cg.isObject())
                continue;

            const QJsonObject &cg = val_cg.toObject();

            // include paths
            if (!(cg.contains("includes") && cg["includes"].isArray()))
                continue;

            const QJsonArray &target_includes = cg["includes"].toArray();
            for (const QJsonValue &val : std::as_const(target_includes)) {
                if (!val["path"].isString())
                    continue;
                targetInfo->includes.append(Utils::FilePath::fromString(val["path"].toString()));
            }

            // defines
            if (!(cg.contains("defines") && cg["defines"].isArray()))
                continue;

            const QJsonArray &target_defines = cg["defines"].toArray();
            for (const QJsonValue &val : std::as_const(target_defines)) {
                if (!val["define"].isString())
                    continue;
                targetInfo->defines.append(val["define"].toString());
            }

            // flags
            if (!(cg.contains("compileCommandFragments") && cg["compileCommandFragments"].isArray()))
                continue;

            const QJsonArray &target_flags = cg["compileCommandFragments"].toArray();
            QStringList flags;
            for (const QJsonValue &val : std::as_const(target_flags)) {
                if (!val["fragment"].isString())
                    continue;
                flags.append(val["fragment"].toString());
                targetInfo->flags.append(val["fragment"].toString());
            }
        }

        package.targets.append(targetInfo);
    }

    return true;
}

QMap<QString, QString> ROSUtils::getROSPackages(const QStringList &env)
{
  QProcess process;
  QMap<QString, QString> package_map;
  QStringList tmp;

  process.setEnvironment(env);
  process.start(QStringLiteral("bash"), QStringList());
  process.waitForStarted();
  QString cmd = QStringLiteral("rospack list"); // TODO: for ROS2 do 'ros2 pkg list'
  process.write(cmd.toLatin1());
  process.closeWriteChannel();
  process.waitForFinished();

  if (process.exitStatus() != QProcess::CrashExit)
  {
    QString output = QString::fromStdString(process.readAllStandardOutput().toStdString());
    static const QRegularExpression ex_newline("[\r\n]");
    QStringList package_list = output.split(ex_newline, Qt::SkipEmptyParts);

    for (const QString& str : std::as_const(package_list))
    {
        tmp = str.split(QStringLiteral(" "));
        package_map.insert(tmp[0],tmp[1]);
    }

    return package_map;
  }
  return QMap<QString, QString>();
}

QMap<QString, QString> ROSUtils::getWorkspacePackagePaths(const WorkspaceInfo &workspaceInfo)
{
    QMap<QString, QString> packageMap;

    const QDir srcDir(workspaceInfo.sourcePath.toFSPathString());
    if(srcDir.exists())
    {
      QDirIterator it(srcDir.absolutePath(),QStringList() << QStringLiteral("package.xml"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
      while (it.hasNext())
      {
        QFileInfo packageFile(it.next());
        packageMap.insert(packageFile.absoluteDir().dirName(), packageFile.absoluteDir().absolutePath());
      }
    }
    else
    {
        Core::MessageManager::writeFlashing(QObject::tr("[ROS Error] Workspace source directory does not exist: %1.").arg(workspaceInfo.sourcePath.toFSPathString()));
    }

    return packageMap;
}

QMap<QString, QString> ROSUtils::getROSPackageLaunchFiles(const QString &packagePath)
{
  QMap<QString, QString> launchFiles;
  if(!packagePath.isEmpty())
  {
    const QDir srcDir(packagePath);
    QDirIterator it(srcDir.absolutePath(),QStringList() << QStringLiteral("*.launch"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

    while (it.hasNext())
    {
      QFileInfo launchFile(it.next());
      launchFiles.insert(launchFile.fileName(), launchFile.absoluteFilePath());
    }
  }

  return launchFiles;
}

QMap<QString, QString> ROSUtils::getROSPackageExecutables(const QString &packageName, const QStringList &env)
{

  QProcess process;
  QMap<QString, QString> package_executables;

  process.setEnvironment(env);
  process.start(QStringLiteral("bash"), QStringList());
  process.waitForStarted();
  QString cmd = QStringLiteral("catkin_find --without-underlays --libexec ") + packageName;
  process.write(cmd.toLatin1());
  process.closeWriteChannel();
  process.waitForFinished();

  if (process.exitStatus() != QProcess::CrashExit)
  {
    QString output = QString::fromStdString(process.readAllStandardOutput().toStdString());
    static const QRegularExpression ex_newline("[\r\n]");
    QStringList loc_list = output.split(ex_newline, Qt::SkipEmptyParts);

    if (loc_list.size() > 0)
    {
      const QDir srcDir(loc_list[0]);
      QDirIterator it(srcDir.absolutePath(), QDir::Files | QDir::Executable | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);

      while (it.hasNext())
      {
        QFileInfo executableFile(it.next());
        package_executables.insert(executableFile.fileName(), executableFile.absoluteFilePath());
      }

      return package_executables;
    }
  }

  return QMap<QString, QString>();
}

Utils::FilePath ROSUtils::getCatkinToolsProfilesPath(const Utils::FilePath &workspaceDir)
{
    Utils::FilePath profiles(workspaceDir);
    profiles = profiles.pathAppended(QStringLiteral(".catkin_tools"));
    profiles = profiles.pathAppended(QStringLiteral("profiles"));
    return profiles;
}

Utils::FilePath ROSUtils::getCatkinToolsProfilesYamlFile(const Utils::FilePath &workspaceDir)
{
    Utils::FilePath profiles = getCatkinToolsProfilesPath(workspaceDir);
    profiles = profiles.pathAppended(QStringLiteral("profiles.yaml"));
    return profiles;
}

bool ROSUtils::setCatkinToolsProfilesYamlFile(const Utils::FilePath &workspaceDir,
                                              const QString &profileName)
{
    YAML::Node config;

    Utils::FilePath profiles = getCatkinToolsProfilesYamlFile(workspaceDir);

    if (profiles.exists())
        config = YAML::LoadFile(profiles.toFSPathString().toStdString());

    config["active"] = profileName.toStdString();

    std::ofstream fout(profiles.toFSPathString().toStdString());

    if( ! fout.is_open())
        return false;

    fout << config; // dump it back into the file

    return true;
}

Utils::FilePath ROSUtils::getCatkinToolsProfilePath(const Utils::FilePath &workspaceDir, const QString &profileName)
{
    Utils::FilePath profile = getCatkinToolsProfilesPath(workspaceDir);
    profile = profile.pathAppended(profileName);
    return profile;
}

Utils::FilePath ROSUtils::getCatkinToolsProfileConfigFile(const Utils::FilePath &workspaceDir, const QString &profileName)
{
    Utils::FilePath profile = getCatkinToolsProfilePath(workspaceDir, profileName);
    profile = profile.pathAppended("config.yaml");
    return profile;
}

bool ROSUtils::isCatkinToolsProfileConfigValid(const Utils::FilePath& configPath)
{
  if (!configPath.exists())
    return false;

  YAML::Node config = YAML::LoadFile(configPath.toFSPathString().toStdString());
  if (config.IsNull())
    return false;

  if (!config["source_space"].IsDefined())
    return false;

  if (!config["build_space"].IsDefined())
    return false;

  if (!config["devel_space"].IsDefined())
    return false;

  if (!config["install_space"].IsDefined())
    return false;

  if (!config["log_space"].IsDefined())
    return false;

  if (!config["install"].IsDefined())
    return false;

  return true;
}

bool ROSUtils::removeCatkinToolsProfile(const Utils::FilePath &workspaceDir, const QString &profileName)
{
    QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);

    if( activeProfile.length() )
    {
        Utils::FilePath profiles = getCatkinToolsProfilePath(workspaceDir, profileName);
        QDir d(profiles.toFSPathString());
        if (d.exists())
        {
            if (!d.removeRecursively())
                return false;

            if (activeProfile == profileName)
                setCatkinToolsActiveProfile(workspaceDir, QStringLiteral("default"));
        }
    }

    return true;
}

bool ROSUtils::renameCatkinToolsProfile(const Utils::FilePath &workspaceDir, const QString &oldProfileName, const QString &newProfileName)
{
    Utils::FilePath profile = getCatkinToolsProfilePath(workspaceDir, oldProfileName);
    QDir d(profile.toFSPathString());
    if (d.exists())
        return d.rename(oldProfileName, newProfileName);

    return createCatkinToolsProfile(workspaceDir, newProfileName, true);
}

bool ROSUtils::createCatkinToolsProfile(const Utils::FilePath &workspaceDir, const QString &profileName, bool overwrite)
{
    Utils::FilePath config = getCatkinToolsProfileConfigFile(workspaceDir, profileName);

    QDir().mkpath(getCatkinToolsProfilePath(workspaceDir, profileName).toFSPathString());
    if (overwrite)
      QFile::remove(config.toFSPathString());

    return (QFile::copy(":rosproject/config.yaml", config.toFSPathString()) &&
            QFile::setPermissions(config.toFSPathString(),
                                  QFile::ReadUser |
                                  QFile::WriteUser |
                                  QFile::ReadGroup |
                                  QFile::WriteGroup));
}

bool ROSUtils::cloneCatkinToolsProfile(const Utils::FilePath &workspaceDir, const QString &profileName, const QString &newProfileName)
{
    Utils::FilePath copyConfig = getCatkinToolsProfileConfigFile(workspaceDir, profileName);
    Utils::FilePath newConfig = getCatkinToolsProfileConfigFile(workspaceDir, newProfileName);

    if (!isCatkinToolsProfileConfigValid(copyConfig))
        return createCatkinToolsProfile(workspaceDir, profileName, true);

    QDir().mkpath(getCatkinToolsProfilePath(workspaceDir, newProfileName).toFSPathString());
    return QFile::copy(copyConfig.toFSPathString(), newConfig.toFSPathString());
}

QString ROSUtils::getCatkinToolsActiveProfile(const Utils::FilePath &workspaceDir)
{
    QString activeProfile;
    Utils::FilePath profiles = getCatkinToolsProfilesYamlFile(workspaceDir);
    if (profiles.exists())
    {
        YAML::Node config = YAML::LoadFile(profiles.toFSPathString().toStdString());
        activeProfile = QString::fromStdString(config["active"].as<std::string>());
    }
    else
        return QString("");

    return activeProfile;
}

QString ROSUtils::setCatkinToolsDefaultProfile(const Utils::FilePath &workspaceDir)
{
    QString defaultProfile("default");

    // This will create the profile if it does not exist with default config.
    setCatkinToolsActiveProfile(workspaceDir, defaultProfile);

    return defaultProfile;
}

bool ROSUtils::setCatkinToolsActiveProfile(const Utils::FilePath &workspaceDir, const QString &profileName)
{
    // Create profiles directory if it does not exist
    QDir().mkpath(getCatkinToolsProfilesPath(workspaceDir).toFSPathString());

    if( ! setCatkinToolsProfilesYamlFile(workspaceDir, profileName) )
        return false;

    // This will create the profile if it does not exist with default config.
    createCatkinToolsProfile(workspaceDir, profileName, false);

    return true;
}

QStringList ROSUtils::getCatkinToolsProfileNames(const Utils::FilePath &workspaceDir)
{
    Utils::FilePath profiles = getCatkinToolsProfilesPath(workspaceDir);
    if (profiles.exists())
    {
        QDir d(profiles.toFSPathString());
        QStringList profileNames = d.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        if (!profileNames.empty())
            return profileNames;
    }

    // If there are currently no profiles, create a default profile.
    createCatkinToolsProfile(workspaceDir, QStringLiteral("default"), true);
    return QStringList() << QStringLiteral("default");
}

Utils::FilePath ROSUtils::getCatkinToolsProfile(const Utils::FilePath &workspaceDir, const QString &profileName)
{
    Utils::FilePath profile = ROSUtils::getCatkinToolsProfileConfigFile(workspaceDir, profileName);
    if(!isCatkinToolsProfileConfigValid(profile))
        createCatkinToolsProfile(workspaceDir, profileName, true);

    return profile;
}

QString ROSUtils::getCMakeBuildTypeArgument(ROSUtils::BuildType &buildType)
{
    switch (buildType) {
    case ROSUtils::BuildTypeDebug:
        return QStringLiteral("-DCMAKE_BUILD_TYPE=Debug");
    case ROSUtils::BuildTypeMinSizeRel:
        return QStringLiteral("-DCMAKE_BUILD_TYPE=MinSizeRel");
    case ROSUtils::BuildTypeRelWithDebInfo:
        return QStringLiteral("-DCMAKE_BUILD_TYPE=RelWithDebInfo");
    case ROSUtils::BuildTypeRelease:
        return QStringLiteral("-DCMAKE_BUILD_TYPE=Release");
    default:
        return QString();
    }
}

ROSUtils::WorkspaceInfo ROSUtils::getWorkspaceInfo(const Utils::FilePath &workspaceDir,
                                                   const BuildSystem buildSystem,
                                                   const Utils::FilePath &rosDistribution)
{
    WorkspaceInfo space;
    space.path = workspaceDir;
    space.buildSystem = buildSystem;
    space.rosDistribution = rosDistribution;

    switch(buildSystem) {
    case CatkinMake:
    {
        space.sourcePath = Utils::FilePath(workspaceDir).pathAppended("src");
        space.buildPath = Utils::FilePath(workspaceDir).pathAppended("build");
        space.develPath = Utils::FilePath(workspaceDir).pathAppended("devel");
        space.installPath = Utils::FilePath(workspaceDir).pathAppended("install");
        space.logPath = Utils::FilePath(workspaceDir).pathAppended("logs");
        space.install = false; //TODO: Need to find how best to determine if installing
        break;
    }
    case CatkinTools:
    {
        YAML::Node config;
        QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);

        if( activeProfile.length() )
        {
            Utils::FilePath configPath = getCatkinToolsProfileConfigFile(workspaceDir, activeProfile);
            if (!isCatkinToolsProfileConfigValid(configPath))
              createCatkinToolsProfile(workspaceDir, activeProfile, true);

            config = YAML::LoadFile(configPath.toFSPathString().toStdString());
            space.sourcePath = Utils::FilePath(workspaceDir).pathAppended(QString::fromStdString(config["source_space"].as<std::string>()));
            space.buildPath = Utils::FilePath(workspaceDir).pathAppended(QString::fromStdString(config["build_space"].as<std::string>()));
            space.develPath = Utils::FilePath(workspaceDir).pathAppended(QString::fromStdString(config["devel_space"].as<std::string>()));
            space.installPath = Utils::FilePath(workspaceDir).pathAppended(QString::fromStdString(config["install_space"].as<std::string>()));
            space.logPath = Utils::FilePath(workspaceDir).pathAppended(QString::fromStdString(config["log_space"].as<std::string>()));
            space.install = config["install"].as<bool>();
        }
        else
        {
            // TODO: This is temporary fix since at time of CatkinTools.workspace creation
            // <Directory>.</Directory> is created instead of <Directory>src</Directory>
            // which causes random crash in file watcher reading "." instead of "src"
            space.sourcePath = Utils::FilePath(workspaceDir).pathAppended(QString::fromStdString("src"));
        }
        break;
    }
    case Colcon:
    {
        space.sourcePath = Utils::FilePath(workspaceDir).pathAppended("src");
        space.buildPath = Utils::FilePath(workspaceDir).pathAppended("build");
        space.develPath = Utils::FilePath(workspaceDir).pathAppended("install"); // Colcon does not have devel space setting to install
        space.installPath = Utils::FilePath(workspaceDir).pathAppended("install");
        space.logPath = Utils::FilePath(workspaceDir).pathAppended("log");
        space.install = true; // Calcon always uses the install space.
        break;
    }
    }

    return space;
}

QProcessEnvironment ROSUtils::getWorkspaceEnvironment(const WorkspaceInfo &workspaceInfo, const Utils::Environment& current_environment)
{
    // initialise environment
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(current_environment.toProcessEnvironment());
    env.insert("PWD", workspaceInfo.path.toFSPathString());

    // source workspaces
    sourceWorkspace(env, workspaceInfo);

    return env;
}

bool ROSUtils::findPackageBuildDirectory(const WorkspaceInfo &workspaceInfo, const PackageInfo &packageInfo, Utils::FilePath &packageBuildPath)
{
    packageBuildPath = workspaceInfo.buildPath;
    switch(workspaceInfo.buildSystem) {
    case CatkinMake:
    {
        QString diff = packageInfo.path.toFSPathString();
        diff.remove(workspaceInfo.sourcePath.toFSPathString());
        packageBuildPath = packageBuildPath.stringAppended(diff);
        break;
    }
    case CatkinTools:
    {
        packageBuildPath = workspaceInfo.buildPath.pathAppended(packageInfo.name);
        break;
    }
    case Colcon:
    {
        packageBuildPath = workspaceInfo.buildPath.pathAppended(packageInfo.name);
        break;
    }
    }

    if (!QDir(packageBuildPath.toFSPathString()).exists())
        return false;

    return true;
}

bool ROSUtils::PackageInfo::exists() const
{
    return QDir(path.toFSPathString()).exists();
}

bool ROSUtils::PackageBuildInfo::exists() const
{
    return QDir(path.toFSPathString()).exists();
}

} //namespace Internal
} //namespace ROSProjectManager
