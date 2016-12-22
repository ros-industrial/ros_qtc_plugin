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

#include <utils/fileutils.h>
#include <utils/environment.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>

namespace ROSProjectManager {
namespace Internal {

ROSUtils::ROSUtils()
{

}

QString ROSUtils::buildTypeName(const ROSUtils::BuildType &buildType)
{
    switch (buildType) {
    case ROSUtils::BuildTypeDebug:
        return QLatin1String("Debug");
    case ROSUtils::BuildTypeMinSizeRel:
        return QLatin1String("Minimum Size Release");
    case ROSUtils::BuildTypeRelWithDebInfo:
        return QLatin1String("Release with Debug Information");
    case ROSUtils::BuildTypeRelease:
        return QLatin1String("Release");
    default:
        return QLatin1String("Release");
    }
}

bool ROSUtils::sourceROS(QProcess *process, const QString &rosDistribution)
{
  bool results = sourceWorkspaceHelper(process, Utils::FileName::fromString(QLatin1String(ROSProjectManager::Constants::ROS_INSTALL_DIRECTORY)).appendPath(rosDistribution).appendPath(QLatin1String("setup.bash")).toString());
  if (!results)
    qDebug() << "Faild to source ROS Distribution: " << rosDistribution;

  return results;
}

bool ROSUtils::sourceWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution, const BuildSystem buildSystem)
{
    if (!initializeWorkspace(process, workspaceDir, rosDistribution, buildSystem))
        return false;

    Utils::FileName devDir = getWorkspaceDevelSpace(workspaceDir, buildSystem);
    if (sourceWorkspaceHelper(process, devDir.appendPath(QLatin1String("setup.bash")).toString()))
        return true;

    qDebug() << "Failed to source workspace: " << workspaceDir.toString();
    return false;
}

bool ROSUtils::isWorkspaceInitialized(const Utils::FileName &workspaceDir, const ROSUtils::BuildSystem buildSystem)
{
    switch (buildSystem) {
    case ROSUtils::CatkinMake:
    {
        Utils::FileName topCMake = getWorkspaceSourceSpace(workspaceDir, buildSystem);
        topCMake.appendPath(QLatin1String("CMakeLists.txt"));
        Utils::FileName catkin_workspace(workspaceDir);
        catkin_workspace.appendPath(QLatin1String(".catkin_workspace"));

        if (topCMake.exists() || catkin_workspace.exists())
          return true;

        return false;
    }
    case ROSUtils::CatkinTools:
    {
        Utils::FileName catkin_tools(workspaceDir);
        catkin_tools.appendPath(QLatin1String(".catkin_tools"));
        if (catkin_tools.exists())
          return true;

        return false;
    }
    }

    return false;
}

bool ROSUtils::initializeWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution, const BuildSystem buildSystem)
{
    if (sourceROS(process, rosDistribution))
        if (!isWorkspaceInitialized(workspaceDir, buildSystem))
        {
            switch (buildSystem) {
            case CatkinMake:
            {
                Utils::FileName srcDir = getWorkspaceSourceSpace(workspaceDir, buildSystem);

                if (!srcDir.exists())
                {
                    QDir().mkpath(srcDir.toString());
                }
                process->setWorkingDirectory(srcDir.toString());
                process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin_init_workspace"));
                process->waitForFinished();
                break;
            }
            case CatkinTools:
            {
                process->setWorkingDirectory(workspaceDir.toString());
                process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin init"));
                process->waitForFinished();
                break;
            }
            }

            if (process->exitStatus() != QProcess::CrashExit)
                return buildWorkspace(process, workspaceDir, buildSystem);

            qDebug() << "Failed ot initialize workspace: " << workspaceDir.toString();
            return false;
        }

    return true;
}

bool ROSUtils::buildWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const ROSUtils::BuildSystem buildSystem)
{
    switch(buildSystem) {
    case CatkinMake:
    {
        process->setWorkingDirectory(workspaceDir.toString());
        process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin_make --cmake-args -G \"CodeBlocks - Unix Makefiles\""));
        process->waitForFinished();
        break;
    }
    case CatkinTools:
    {
        process->setWorkingDirectory(workspaceDir.toString());
        process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin build --cmake-args -G \"CodeBlocks - Unix Makefiles\""));
        process->waitForFinished();
        break;
    }
    }

    if (process->exitStatus() != QProcess::CrashExit)
        return true;

    qDebug() << "Failed ot build workspace: " << workspaceDir.toString();
    return false;
}

QStringList ROSUtils::installedDistributions()
{
  QDir ros_opt(QLatin1String(ROSProjectManager::Constants::ROS_INSTALL_DIRECTORY));
  ros_opt.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
  QStringList distributions = ros_opt.entryList();

  if (distributions.isEmpty())
      qDebug() << "ROS Does not appear to be installed.";

  return distributions;
}

bool ROSUtils::sourceWorkspaceHelper(QProcess *process, const QString &path)
{
  QStringList env_list;

  process->start(QLatin1String("bash"));
  process->waitForStarted();
  QString cmd = QLatin1String("source ") + path + QLatin1String(" && env > /tmp/rosqtenv.txt");
  process->write(cmd.toLatin1());
  process->closeWriteChannel();
  process->waitForFinished();

  if (process->exitStatus() != QProcess::CrashExit)
  {
    QFile env_file(QLatin1String("/tmp/rosqtenv.txt"));
    if (env_file.open(QIODevice::ReadOnly))
    {
      QTextStream env_stream(&env_file);
      while (!env_stream.atEnd())
      {
        env_list << env_stream.readLine();
      }
      env_file.close();
      Utils::Environment env(env_list);
      process->setProcessEnvironment(env.toProcessEnvironment());
      return true;
    }
  }
  return false;
}

bool ROSUtils::gererateQtCreatorWorkspaceFile(QXmlStreamWriter &xmlFile, const QString distribution, const QStringList &watchDirectories)
{
  xmlFile.setAutoFormatting(true);
  xmlFile.writeStartDocument();
  xmlFile.writeStartElement(QLatin1String("Workspace"));

  xmlFile.writeStartElement(QLatin1String("Distribution"));
  xmlFile.writeAttribute(QLatin1String("name"), distribution);
  xmlFile.writeEndElement();

  xmlFile.writeStartElement(QLatin1String("WatchDirectories"));
  foreach (QString str, watchDirectories)
  {
    xmlFile.writeTextElement(QLatin1String("Directory"), str);
  }
  xmlFile.writeEndElement();

  xmlFile.writeEndElement();
  xmlFile.writeEndDocument();
  return xmlFile.hasError();
}

QHash<QString, ROSUtils::FolderContent> ROSUtils::getFolderContent(const Utils::FileName &folderPath, QStringList &fileList)
{
  QHash<QString, ROSUtils::FolderContent> workspaceFiles;
  ROSUtils::FolderContent content;
  QString folder = folderPath.toString();

    // Get Directory data
    content.directories = QDir(folder).entryList(QDir::NoDotAndDotDot  | QDir::Dirs);
    content.files = QDir(folder).entryList(QDir::NoDotAndDotDot  | QDir::Files);
    workspaceFiles[folder] = content;

    foreach (QString file, content.files)
        fileList.append(QDir(folder).absoluteFilePath(file));

    // Get SubDirectory Information
    const QDir subDir(folder);
    QDirIterator itSrc(subDir.absolutePath(), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (itSrc.hasNext())
    {
        folder = itSrc.next();
        content.directories = QDir(folder).entryList(QDir::NoDotAndDotDot  | QDir::Dirs);
        content.files = QDir(folder).entryList(QDir::NoDotAndDotDot  | QDir::Files);
        workspaceFiles[folder] = content;

        foreach (QString file, content.files)
            fileList.append(QDir(folder).absoluteFilePath(file));
    }

    return workspaceFiles;
}

QMap<QString, ROSUtils::PackageInfo> ROSUtils::getWorkspacePackageInfo(const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{
    QMap<QString, ROSUtils::PackageInfo> wsPackageInfo;
    QMap<QString, QString> packages =  ROSUtils::getWorkspacePackages(workspaceDir, buildSystem);
    QMap<QString, QString> cbp = ROSUtils::getWorkspaceCodeBlockFiles(workspaceDir, buildSystem);

    QMapIterator<QString, QString> it(packages);
    while (it.hasNext())
    {
        it.next();
        ROSUtils::PackageInfo package;
        package.name = it.key();
        package.path = it.value();
        package.filepath = Utils::FileName::fromString(package.path).appendPath("package.xml").toString();
        if (cbp.contains(package.name))
        {
            package.cbpFile = cbp[package.name];
            if (!ROSUtils::getPackageBuildInfo(workspaceDir, buildSystem, package))
                qDebug() << QString("Failed to parse CodeBlocks file: %1").arg(package.cbpFile);
        }
        else
        {
            qDebug() << QString("Unable to locate CodeBlocks file for package: %1").arg(package.name);
        }
        wsPackageInfo.insert(package.name, package);
    }
    return wsPackageInfo;
}

bool ROSUtils::getPackageBuildInfo(const Utils::FileName &workspaceDir, const BuildSystem buildSystem, ROSUtils::PackageInfo &package)
{

  // Parse CodeBlocks Project File
  // Need to search for all of the tags <Add directory="include path" />
  QStringList workspace_includes;
  QStringList system_includes;
  QXmlStreamReader cbpXml;

  QFile cbpFile(package.cbpFile);
  if (!cbpFile.open(QFile::ReadOnly | QFile::Text))
  {
    qDebug() << "Error opening CodeBlocks Project File";
    return false;
  }

  cbpXml.setDevice(&cbpFile);
  cbpXml.readNext();
  while(!cbpXml.atEnd())
  {
    if(cbpXml.isStartElement())
    {
      if(cbpXml.name() == QLatin1String("Add"))
      {
        foreach(const QXmlStreamAttribute &attr, cbpXml.attributes())
        {
          if(attr.name().toString() == QLatin1String("directory"))
          {
            QString attribute_value = attr.value().toString();
            if (attribute_value.startsWith(workspaceDir.toString()))
            {
              if (!workspace_includes.contains(attribute_value))
                workspace_includes.append(attribute_value);
            }
            else if(!system_includes.contains(attribute_value))
            {
              system_includes.append(attribute_value);
            }
          }
        }
      }
    }
    cbpXml.readNext();
  }

  // Next search the package directory for any missed include folders
  QString includePath;
  QDirIterator itPackage(package.path, QStringList() << QLatin1String("include"), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (itPackage.hasNext())
  {
    includePath = itPackage.next();
    if(!workspace_includes.contains(includePath))
      workspace_includes.append(includePath);
  }

  // Add devel include directory
  Utils::FileName develPath = getWorkspaceDevelSpace(workspaceDir, buildSystem);;
  workspace_includes.append(develPath.appendPath(QLatin1String("include")).toString());

  // The order matters so it will order local first then system
  workspace_includes.append(system_includes);

  package.includes = workspace_includes;
  package.flags = QStringList() << QLatin1String("-std=c++11"); // TODO: Need to get this from the CodeBlock File
  return true;
}

QMap<QString, QString> ROSUtils::getROSPackages(const QStringList &env)
{
  QProcess process;
  QMap<QString, QString> package_map;
  QStringList tmp;

  process.setEnvironment(env);
  process.start(QLatin1String("bash"));
  process.waitForStarted();
  QString cmd = QLatin1String("rospack list > /tmp/rosqtpackages.txt");
  process.write(cmd.toLatin1());
  process.closeWriteChannel();
  process.waitForFinished();

  if (process.exitStatus() != QProcess::CrashExit)
  {
    QFile package_file(QLatin1String("/tmp/rosqtpackages.txt"));
    if (package_file.open(QIODevice::ReadOnly))
    {
      QTextStream package_stream(&package_file);
      while (!package_stream.atEnd())
      {
        tmp = package_stream.readLine().split(QLatin1String(" "));
        package_map.insert(tmp[0],tmp[1]);
      }
      package_file.close();
      return package_map;
    }
  }
  return QMap<QString, QString>();
}

QMap<QString, QString> ROSUtils::getWorkspacePackages(const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{
    QMap<QString, QString> packageMap;
    Utils::FileName srcPath = getWorkspaceSourceSpace(workspaceDir, buildSystem);

    const QDir srcDir(srcPath.toString());
    if(srcDir.exists())
    {
      QDirIterator it(srcDir.absolutePath(),QStringList() << QLatin1String("package.xml"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
      while (it.hasNext())
      {
        QFileInfo packageFile(it.next());
        packageMap.insert(packageFile.absoluteDir().dirName(), packageFile.absoluteDir().absolutePath());
      }
    }
    else
    {
        qDebug() << QString("Directory does not exist: %1").arg(srcPath.toString());
    }

    return packageMap;
}

QMap<QString, QString> ROSUtils::getWorkspaceCodeBlockFiles(const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{
    QMap<QString, QString> cbpMap;
    Utils::FileName buildPath = getWorkspaceBuildSpace(workspaceDir, buildSystem);
    const QDir buildDir(buildPath.toString());
    if(buildDir.exists())
    {
      QDirIterator it(buildDir.absolutePath(),QStringList() << QLatin1String("*.cbp"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
      while (it.hasNext())
      {
        QFileInfo packageFile(it.next());
        cbpMap.insert(packageFile.baseName(), packageFile.absoluteFilePath());
      }
    }
    else
    {
        qDebug() << QString("Directory does not exist: %1").arg(buildPath.toString());
    }

    return cbpMap;
}

QStringList ROSUtils::getROSPackageLaunchFiles(const QString &packagePath, bool OnlyNames)
{
  QStringList launchFiles;
  if(!packagePath.isEmpty())
  {
    const QDir srcDir(packagePath);
    QDirIterator it(srcDir.absolutePath(),QStringList() << QLatin1String("*.launch"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
      QFileInfo launchFile(it.next());
      if(OnlyNames)
        launchFiles.append(launchFile.fileName());
      else
        launchFiles.append(launchFile.absoluteFilePath());
    }
  }

  return launchFiles;
}

QStringList ROSUtils::getROSPackageExecutables(const QString &packageName, const QStringList &env)
{

  QProcess process;
  QStringList package_executables;
  QString package_executables_location;

  process.setEnvironment(env);
  process.start(QLatin1String("bash"));
  process.waitForStarted();
  QString cmd = QLatin1String("catkin_find --without-underlays --libexec ") + packageName + QLatin1String(" > /tmp/rosqtexecutables.txt");
  process.write(cmd.toLatin1());
  process.closeWriteChannel();
  process.waitForFinished();

  if (process.exitStatus() != QProcess::CrashExit)
  {
    QFile executable_file(QLatin1String("/tmp/rosqtexecutables.txt"));
    if (executable_file.open(QIODevice::ReadOnly))
    {
      QTextStream executable_stream(&executable_file);
      if (!executable_stream.atEnd())
      {
        package_executables_location = executable_stream.readLine();
      }
      executable_file.close();

      if(!package_executables_location.isEmpty())
      {
        const QDir srcDir(package_executables_location);
        QDirIterator it(srcDir.absolutePath(), QDir::Files | QDir::Executable | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

        while (it.hasNext())
        {
          QFileInfo executableFile(it.next());
          package_executables.append(executableFile.fileName());
        }

        return package_executables;
      }
    }
  }

  return QStringList();
}

Utils::FileName ROSUtils::getCatkinToolsProfilesPath(const Utils::FileName &workspaceDir)
{
    Utils::FileName profiles(workspaceDir);
    profiles.appendPath(QLatin1String(".catkin_tools"));
    profiles.appendPath(QLatin1String("profiles"));
    return profiles;
}

Utils::FileName ROSUtils::getCatkinToolsProfilesYamlFile(const Utils::FileName &workspaceDir)
{
    Utils::FileName profiles = getCatkinToolsProfilesPath(workspaceDir);
    profiles.appendPath(QLatin1String("profiles.yaml"));
    return profiles;
}

Utils::FileName ROSUtils::getCatkinToolsProfilePath(const Utils::FileName &workspaceDir, const QString profileName)
{
    Utils::FileName profile = getCatkinToolsProfilesPath(workspaceDir);
    profile.appendPath(profileName);
    return profile;
}

Utils::FileName ROSUtils::getCatkinToolsProfileConfigFile(const Utils::FileName &workspaceDir, const QString profileName)
{
    Utils::FileName profile = getCatkinToolsProfilePath(workspaceDir, profileName);
    profile.appendPath("config.yaml");
    return profile;
}

bool ROSUtils::removeCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString profileName)
{

    QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);

    Utils::FileName profiles = getCatkinToolsProfilePath(workspaceDir, profileName);
    QDir d(profiles.toString());
    if (d.exists())
    {
        if (!d.removeRecursively())
            return false;

        if (activeProfile == profileName)
            setCatkinToolsActiveProfile(workspaceDir, QLatin1Literal("default"));

    }

    return true;
}

bool ROSUtils::renameCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString &oldProfileName, const QString &newProfileName)
{
    Utils::FileName profile = getCatkinToolsProfilePath(workspaceDir, oldProfileName);
    QDir d(profile.toString());
    if (d.exists())
        return d.rename(oldProfileName, newProfileName);

    return createCatkinToolsProfile(workspaceDir, newProfileName);
}

bool ROSUtils::createCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString profileName)
{
    Utils::FileName config = getCatkinToolsProfileConfigFile(workspaceDir, profileName);
    if (!config.exists())
    {
        QDir().mkpath(getCatkinToolsProfilePath(workspaceDir, profileName).toString());
        return (QFile::copy(":rosproject/config.yaml", config.toString()) &&
                QFile::setPermissions(config.toString(),
                                      QFile::ReadUser |
                                      QFile::WriteUser |
                                      QFile::ReadGroup |
                                      QFile::WriteGroup));
    }

    return true;
}

bool ROSUtils::cloneCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString &profileName, const QString &newProfileName)
{
    Utils::FileName copyConfig = getCatkinToolsProfileConfigFile(workspaceDir, profileName);
    Utils::FileName newConfig = getCatkinToolsProfileConfigFile(workspaceDir, newProfileName);

    if (!copyConfig.exists())
        return createCatkinToolsProfile(workspaceDir, profileName);

    QDir().mkpath(getCatkinToolsProfilePath(workspaceDir, newProfileName).toString());
    return QFile::copy(copyConfig.toString(), newConfig.toString());
}

QString ROSUtils::getCatkinToolsActiveProfile(const Utils::FileName &workspaceDir)
{
    QString activeProfile;
    Utils::FileName profiles = getCatkinToolsProfilesYamlFile(workspaceDir);
    if (profiles.exists())
    {
        YAML::Node config = YAML::LoadFile(profiles.toString().toStdString());
        activeProfile = QString::fromStdString(config["active"].as<std::string>());
    }
    else
    {
        activeProfile = QLatin1String("default");
    }

    // This will create the profile if it does not exist with default config.
    createCatkinToolsProfile(workspaceDir, activeProfile);

    return activeProfile;
}

bool ROSUtils::setCatkinToolsActiveProfile(const Utils::FileName &workspaceDir, const QString profileName)
{
    YAML::Node config;
    Utils::FileName profiles = getCatkinToolsProfilesYamlFile(workspaceDir);
    if (profiles.exists())
        config = YAML::LoadFile(profiles.toString().toStdString());
    else
        config = YAML::Load("active: " + profileName.toStdString());

    // Create profiles directory if it does not exist
    QDir().mkpath(getCatkinToolsProfilesPath(workspaceDir).toString());

    config["active"] = profileName.toStdString();
    std::ofstream fout(profiles.toString().toStdString());
    fout << config; // dump it back into the file

    // This will create the profile if it does not exist with default config.
    createCatkinToolsProfile(workspaceDir, profileName);

    return true;
}

QStringList ROSUtils::getCatkinToolsProfileNames(const Utils::FileName &workspaceDir)
{
    Utils::FileName profiles = getCatkinToolsProfilesPath(workspaceDir);
    if (profiles.exists())
    {
        QDir d(profiles.toString());
        QStringList profileNames = d.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
        if (!profileNames.empty())
            return profileNames;
    }

    // If there are currently no profiles, create a default profile.
    createCatkinToolsProfile(workspaceDir, QLatin1String("default"));
    return QStringList() << QLatin1String("default");
}

Utils::FileName ROSUtils::getCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString profileName)
{
    Utils::FileName profile = ROSUtils::getCatkinToolsProfileConfigFile(workspaceDir, profileName);
    if(!profile.exists())
        createCatkinToolsProfile(workspaceDir, profileName);

    return profile;
}

QString ROSUtils::getCMakeBuildTypeArgument(ROSUtils::BuildType &buildType)
{
    switch (buildType) {
    case ROSUtils::BuildTypeDebug:
        return QLatin1String("-DCMAKE_BUILD_TYPE=Debug");
    case ROSUtils::BuildTypeMinSizeRel:
        return QLatin1String("-DCMAKE_BUILD_TYPE=MinSizeRel");
    case ROSUtils::BuildTypeRelWithDebInfo:
        return QLatin1String("-DCMAKE_BUILD_TYPE=RelWithDebInfo");
    default:
        return QLatin1String("-DCMAKE_BUILD_TYPE=Release");
    }
}

Utils::FileName ROSUtils::getWorkspaceSourceSpace(const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{
    Utils::FileName space(workspaceDir);
    switch(buildSystem) {
    case CatkinMake:
    {
        space.appendPath("src");
        break;
    }
    case CatkinTools:
    {
        YAML::Node config;
        QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);
        Utils::FileName configPath = getCatkinToolsProfileConfigFile(workspaceDir, activeProfile);
        config = YAML::LoadFile(configPath.toString().toStdString());
        space.appendPath(QString::fromStdString(config["source_space"].as<std::string>()));
        break;
    }
    }

    return space;
}

Utils::FileName ROSUtils::getWorkspaceBuildSpace(const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{
    Utils::FileName space(workspaceDir);
    switch(buildSystem) {
    case CatkinMake:
    {
        space.appendPath("build");
        break;
    }
    case CatkinTools:
    {
        YAML::Node config;
        QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);
        Utils::FileName configPath = getCatkinToolsProfileConfigFile(workspaceDir, activeProfile);
        config = YAML::LoadFile(configPath.toString().toStdString());
        space.appendPath(QString::fromStdString(config["build_space"].as<std::string>()));
        break;
    }
    }

    return space;
}

Utils::FileName ROSUtils::getWorkspaceDevelSpace(const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{
    Utils::FileName space(workspaceDir);
    switch(buildSystem) {
    case CatkinMake:
    {
        space.appendPath("devel");
        break;
    }
    case CatkinTools:
    {
        YAML::Node config;
        QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);
        Utils::FileName configPath = getCatkinToolsProfileConfigFile(workspaceDir, activeProfile);
        config = YAML::LoadFile(configPath.toString().toStdString());
        space.appendPath(QString::fromStdString(config["devel_space"].as<std::string>()));
        break;
    }
    }

    return space;
}

QProcessEnvironment ROSUtils::getWorkspaceEnvironment(const Utils::FileName &workspaceDir, const QString &rosDistribution, const BuildSystem buildSystem)
{
    QProcess process;
    sourceWorkspace(&process, workspaceDir, rosDistribution, buildSystem);
    QProcessEnvironment env = process.processEnvironment();
    env.insert("PWD", workspaceDir.toString());
    env.insert("TERM", "xterm");

    return env;
}

bool ROSUtils::PackageInfo::exists()
{
    return QDir(path).exists();
}

bool ROSUtils::PackageInfo::cbpFileExists()
{
    return QFile(cbpFile).exists();
}

} //namespace Internal
} //namespace ROSProjectManager
