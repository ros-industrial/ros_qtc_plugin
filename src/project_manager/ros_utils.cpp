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

bool ROSUtils::generateCodeBlocksProjectFile(QProcess *process, const Utils::FileName &workspaceDir, const BuildSystem buildSystem)
{

    QString cmd;
    if (buildSystem == CatkinTools)
        cmd = QLatin1String("catkin build");
    else
        cmd = QLatin1String("catkin_make");

    cmd += QLatin1String(" --cmake-args -G \"CodeBlocks - Unix Makefiles\"");
    process->setWorkingDirectory(workspaceDir.toString());
    process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << cmd);
    process->waitForFinished();
    if (process->exitStatus() != QProcess::CrashExit)
    {
      return true;
    }
    else
    {
      qDebug() << "Faild to generate Code Blocks Project File.";
      return false;
    }
}

bool ROSUtils::sourceROS(QProcess *process, const QString &rosDistribution)
{
  bool results = sourceWorkspaceHelper(process, Utils::FileName::fromString(QLatin1String(ROSProjectManager::Constants::ROS_INSTALL_DIRECTORY)).appendPath(rosDistribution).appendPath(QLatin1String("setup.bash")).toString());
  if (!results)
  {
    qDebug() << "Faild to source ROS Distribution: " << rosDistribution;
  }
  return results;
}

bool ROSUtils::sourceWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution, const BuildSystem buildSystem)
{
  bool results = false;
  Utils::FileName devDir(workspaceDir);
  devDir.appendPath(QLatin1String("devel"));

  if (sourceROS(process, rosDistribution))
  {
    results = true;
    if (!isWorkspaceInitialized(workspaceDir, buildSystem))
    {
      results = initializeWorkspace(process, workspaceDir, rosDistribution, buildSystem);
    }

    if (results)
    {
      if (sourceWorkspaceHelper(process, devDir.appendPath(QLatin1String("setup.bash")).toString()))
      {
        results = true;
      }
      else
      {
        results = false;
        qDebug() << "Failed to source workspace: " << workspaceDir.toString();
      }
    }
  }

  return results;
}

bool ROSUtils::isWorkspaceInitialized(const Utils::FileName &workspaceDir, const ROSUtils::BuildSystem buildSystem)
{
    switch (buildSystem) {
    case ROSUtils::CatkinMake:
    {
        Utils::FileName topCMake(workspaceDir);
        topCMake.appendPath(QLatin1String("src")).appendPath(QLatin1String("CMakeLists.txt"));
        Utils::FileName catkin_workspace(workspaceDir);
        catkin_workspace.appendPath(QLatin1String(".catkin_workspace"));

        if (!topCMake.exists() || !catkin_workspace.exists())
        {
          return false;
        }
        else
        {
          return true;
        }
    }
    case ROSUtils::CatkinTools:
    {
        Utils::FileName catkin_tools(workspaceDir);
        catkin_tools.appendPath(QLatin1String(".catkin_tools"));
        if (!catkin_tools.exists())
        {
          return false;
        }
        else
        {
          return true;
        }
    }
    }

    return false;
}

bool ROSUtils::initializeWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution, const BuildSystem buildSystem)
{
    bool results = false;
    if (sourceROS(process, rosDistribution))
    {
        if (!isWorkspaceInitialized(workspaceDir, buildSystem))
        {
            switch (buildSystem) {
            case CatkinMake:
            {
                Utils::FileName srcDir(workspaceDir);
                srcDir.appendPath(QLatin1String("src"));

                if (!srcDir.exists())
                {
                    QDir(workspaceDir.toString()).mkdir(QLatin1String("src"));
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
            {
                results = true;
            }
            else
            {
                results = false;
                qDebug() << "Failed ot initialize workspace: " << workspaceDir.toString();
                return results;
            }

        }
    }

    if (buildWorkspace(process, workspaceDir, buildSystem))
    {
        results = true;
    }
    else
    {
        results = false;
    }


    return results;
}

bool ROSUtils::buildWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const ROSUtils::BuildSystem buildSystem)
{
    switch(buildSystem) {
    case CatkinMake:
    {
        process->setWorkingDirectory(workspaceDir.toString());
        // May need to change PWD Enviroment variable here
        process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin_make"));
        process->waitForFinished();
    }
    case CatkinTools:
    {
        process->setWorkingDirectory(workspaceDir.toString());
        // May need to change PWD Enviroment variable here
        process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin build"));
        process->waitForFinished();
    }
    }

    if (process->exitStatus() != QProcess::CrashExit)
    {
        return true;
    }
    else
    {
        qDebug() << "Failed ot build workspace: " << workspaceDir.toString();
        return false;
    }
}

QStringList ROSUtils::installedDistributions()
{
  QDir ros_opt(QLatin1String(ROSProjectManager::Constants::ROS_INSTALL_DIRECTORY));
  ros_opt.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
  return ros_opt.entryList();
}

bool ROSUtils::sourceWorkspaceHelper(QProcess *process, const QString &path)
{
  bool results = false;
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
      results = true;
    }
  }
  return results;
}

bool ROSUtils::gererateQtCreatorWorkspaceFile(QXmlStreamWriter &xmlFile, const QString distribution, const QStringList &watchDirectories, const QStringList &includePaths)
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

  xmlFile.writeStartElement(QLatin1String("IncludePaths"));
  foreach (const QString &str, includePaths)
  {
    xmlFile.writeTextElement(QLatin1String("Directory"), str);
  }
  xmlFile.writeEndElement();

  xmlFile.writeEndElement();
  xmlFile.writeEndDocument();
  return xmlFile.hasError();
}

QHash<QString, QStringList> ROSUtils::getWorkspaceFiles(const Utils::FileName &workspaceDir)
{
  QHash<QString, QStringList> workspaceFiles;
  Utils::FileName srcPath = workspaceDir;
  srcPath.appendPath(QLatin1String("src"));

  const QDir srcDir(srcPath.toString());
  QString wsDir;
  QDirIterator itSrc(srcDir.absolutePath(), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (itSrc.hasNext())
  {
    wsDir = itSrc.next();
    QFileInfoList dirFiles = QDir(wsDir).entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    if(dirFiles.count() == 0)
    {
      workspaceFiles[wsDir].append(QLatin1Literal("EMPTY_FOLDER"));
    }
    else
    {
      foreach (QFileInfo file, dirFiles)
      {
        workspaceFiles[wsDir].append(file.absoluteFilePath());
      }
    }
  }

  return workspaceFiles;
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
    {
        fileList.append(QDir(folder).absoluteFilePath(file));
    }

    // Get SubDirectory Information
    const QDir srcDir(folder);
    QDirIterator itSrc(srcDir.absolutePath(), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (itSrc.hasNext())
    {
        folder = itSrc.next();
        content.directories = QDir(folder).entryList(QDir::NoDotAndDotDot  | QDir::Dirs);
        content.files = QDir(folder).entryList(QDir::NoDotAndDotDot  | QDir::Files);
        workspaceFiles[folder] = content;

        foreach (QString file, content.files)
        {
            fileList.append(QDir(folder).absoluteFilePath(file));
        }
    }

    return workspaceFiles;
}

QStringList ROSUtils::getWorkspaceIncludes(const Utils::FileName &workspaceDir)
{
  // Parse CodeBlocks Project File
  // Need to search for all of the tags <Add directory="include path" />
  QStringList workspace_includes;
  QStringList system_includes;
  QXmlStreamReader cbpXml;
  Utils::FileName cbpPath = workspaceDir;
  cbpPath.appendPath(QLatin1String("build")).appendPath(QLatin1String("Project.cbp"));
  QFile cbpFile(cbpPath.toString());
  if (!cbpFile.open(QFile::ReadOnly | QFile::Text))
  {
    qDebug() << "Error opening CodeBlocks Project File";
    return workspace_includes;
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

  // Next search the source directory for any missed include folders
  Utils::FileName srcPath = workspaceDir;
  const QDir srcDir(srcPath.toString());
  srcPath.appendPath(QLatin1String("src"));
  QString includePath;
  QDirIterator itSrc(srcDir.absolutePath(),QStringList() << QLatin1String("include"), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (itSrc.hasNext())
  {
    includePath = itSrc.next();
    if(!workspace_includes.contains(includePath))
    {
      workspace_includes.append(includePath);
    }
  }

  // Next search the devel directory for any missed include folders
  Utils::FileName develPath = workspaceDir;
  const QDir develDir(develPath.toString());
  develPath.appendPath(QLatin1String("devel"));
  QDirIterator itDevel(develDir.absolutePath(),QStringList() << QLatin1String("include"), QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (itDevel.hasNext())
  {
    includePath = itDevel.next();
    if(!workspace_includes.contains(includePath))
    {
      workspace_includes.append(includePath);
    }
  }

  // The order matters so it will search local first then system
  workspace_includes.append(system_includes);
  return workspace_includes;
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
      {
        launchFiles.append(launchFile.fileName());
      }
      else
      {
        launchFiles.append(launchFile.absoluteFilePath());
      }
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

Utils::FileName ROSUtils::getCatkinToolsProfilePath(const Utils::FileName &workspaceDir, const QString profileName)
{
    Utils::FileName profile(workspaceDir);
    profile.appendPath(QLatin1String(".catkin_tools"));
    profile.appendPath(QLatin1String("profiles"));
    profile.appendPath(profileName);
    return profile;
}

bool ROSUtils::removeCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString profileName)
{

    QString activeProfile = getCatkinToolsActiveProfile(workspaceDir);

    Utils::FileName profiles(workspaceDir);
    profiles.appendPath(QLatin1String(".catkin_tools"));
    profiles.appendPath(QLatin1String("profiles"));
    profiles.appendPath(profileName);
    if (profiles.exists())
    {
        QDir d(profiles.toString());
        if (!d.removeRecursively())
            return false;

        if (activeProfile == profileName)
            setCatkinToolsActiveProfile(workspaceDir, QLatin1Literal("default"));

    }

    return true;
}

bool ROSUtils::renameCatkinToolsProfile(const Utils::FileName &workspaceDir, const QString &oldProfileName, const QString &newProfileName)
{
    Utils::FileName profile(workspaceDir);
    profile.appendPath(QLatin1String(".catkin_tools"));
    profile.appendPath(QLatin1String("profiles"));
    profile.appendPath(oldProfileName);
    if (profile.exists())
    {
        QDir d(profile.toString());
        return d.rename(oldProfileName, newProfileName);
    }

    return false;
}

QString ROSUtils::getCatkinToolsActiveProfile(const Utils::FileName &workspaceDir)
{
    QString activeProfile;
    Utils::FileName profiles(workspaceDir);
    profiles.appendPath(QLatin1String(".catkin_tools"));
    profiles.appendPath(QLatin1String("profiles"));
    profiles.appendPath(QLatin1String("profiles.yaml"));
    if (profiles.exists())
    {
        YAML::Node config = YAML::LoadFile(profiles.toString().toStdString());
        activeProfile = QString::fromStdString(config["active"].as<std::string>());
    }
    else
    {
        activeProfile = QLatin1String("default");
    }
    return activeProfile;
}

bool ROSUtils::setCatkinToolsActiveProfile(const Utils::FileName &workspaceDir, const QString profileName)
{
    YAML::Node config;
    Utils::FileName profiles(workspaceDir);
    profiles.appendPath(QLatin1String(".catkin_tools"));
    profiles.appendPath(QLatin1String("profiles"));
    profiles.appendPath(QLatin1String("profiles.yaml"));
    if (profiles.exists())
    {
        config = YAML::LoadFile(profiles.toString().toStdString());
    }
    else
    {
        config = YAML::Load("active: default");
    }

    config["active"] = profileName.toStdString();
    std::ofstream fout(profiles.toString().toStdString());
    fout << config; // dump it back into the file
    return true;
}

QStringList ROSUtils::getCatkinToolsProfileNames(const Utils::FileName &workspaceDir)
{
    QStringList profileNames;
    Utils::FileName profiles(workspaceDir);
    profiles.appendPath(QLatin1String(".catkin_tools"));
    profiles.appendPath(QLatin1String("profiles"));
    if (profiles.exists())
    {
        QDir d(profiles.toString());
        profileNames = d.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    }
    return profileNames;
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

} //namespace Internal
} //namespace ROSProjectManager
