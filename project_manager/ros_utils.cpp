#include "ros_utils.h"
#include "ros_project_constants.h"

#include <utils/fileutils.h>
#include <utils/environment.h>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDirIterator>

#include <ros/package.h>

namespace ROSProjectManager {
namespace Internal {

ROSUtils::ROSUtils()
{

}

bool ROSUtils::generateCodeBlocksProjectFile(QProcess *process, const Utils::FileName &sourceDir, const Utils::FileName &buildDir)
{
    QString cmd = QLatin1String("cmake ") + sourceDir.toString() + QLatin1String(" -G \"CodeBlocks - Unix Makefiles\"");
    process->setWorkingDirectory(buildDir.toString());
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

bool ROSUtils::sourceWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution)
{
  bool results = false;
  Utils::FileName ws(workspaceDir);
  Utils::FileName devDir(workspaceDir);
  devDir.appendPath(QLatin1String("devel"));

  if (sourceROS(process, rosDistribution))
  {
    results = true;
    if (!hasBuildDirectory(workspaceDir) || !hasDevelDirectory(workspaceDir))
    {
      results = initializeWorkspace(process, workspaceDir, rosDistribution);
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

bool ROSUtils::isWorkspaceInitialized(const Utils::FileName &workspaceDir)
{
  Utils::FileName topCMake(workspaceDir);
  topCMake.appendPath(QLatin1String("src")).appendPath(QLatin1String("CMakeLists.txt"));
  if (!topCMake.exists())
  {
    return false;
  }
  else
  {
    return true;
  }
}

bool ROSUtils::initializeWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution)
{
  Utils::FileName srcDir(workspaceDir);
  srcDir.appendPath(QLatin1String("src"));

  bool results = false;
  if (sourceROS(process, rosDistribution))
  {
    if (!isWorkspaceInitialized(workspaceDir))
    {
      if (!srcDir.exists())
      {
        QDir(workspaceDir.toString()).mkdir(QLatin1String("src"));
      }
      process->setWorkingDirectory(srcDir.toString());
      process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin_init_workspace"));
      process->waitForFinished();

      if (process->exitStatus() != QProcess::CrashExit)
      {
        results = true;
      }
      else
      {
        results = false;
        qDebug() << "Failed ot initialize workspace: " << workspaceDir.toString();
      }
    }
    else
    {
      results = true;
    }

    if (buildWorkspace(process, workspaceDir))
    {
      results = true;
    }
    else
    {
      results = false;
    }

  }
  return results;
}

bool ROSUtils::hasBuildDirectory(const Utils::FileName &workspaceDir)
{
  Utils::FileName buildDir(workspaceDir);
  buildDir.appendPath(QLatin1String("build"));
  return buildDir.exists();
}

bool ROSUtils::hasDevelDirectory(const Utils::FileName &workspaceDir)
{
  Utils::FileName develDir(workspaceDir);
  develDir.appendPath(QLatin1String("devel"));
  return develDir.exists();
}

bool ROSUtils::buildWorkspace(QProcess *process, const Utils::FileName &workspaceDir)
{
  bool results = false;
  process->setWorkingDirectory(workspaceDir.toString());
  // May need to change PWD Enviroment variable here
  process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << QLatin1String("catkin_make"));
  process->waitForFinished();

  if (process->exitStatus() != QProcess::CrashExit)
  {
    results = true;
  }
  else
  {
    qDebug() << "Failed ot build workspace: " << workspaceDir.toString();
  }

  return results;
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

bool ROSUtils::gererateQtCreatorWorkspaceFile(QXmlStreamWriter &xmlFile, const QStringList &files, const QStringList &includePaths)
{
  xmlFile.setAutoFormatting(true);
  xmlFile.writeStartDocument();
  xmlFile.writeStartElement(QLatin1String("Workspace"));

  xmlFile.writeStartElement(QLatin1String("Files"));
  foreach (const QString &str, files)
  {
    xmlFile.writeTextElement(QLatin1String("File"), str);
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

QStringList ROSUtils::getWorkspaceFiles(const Utils::FileName &workspaceDir)
{
  QStringList workspaceFiles;
  Utils::FileName srcPath = workspaceDir;
  srcPath.appendPath(QLatin1String("src"));

  const QDir srcDir(srcPath.toString());
  QDirIterator it(srcDir.absolutePath(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
  while (it.hasNext())
  {
      workspaceFiles.append(it.next());
  }
  return workspaceFiles;
}

QStringList ROSUtils::getWorkspaceIncludes(const Utils::FileName &workspaceDir)
{
  // Parse CodeBlocks Project File
  // Need to search for all of the tags <Add directory="include path" />
  QStringList includePaths;
  QXmlStreamReader cbpXml;
  Utils::FileName cbpPath = workspaceDir;
  cbpPath.appendPath(QLatin1String("build")).appendPath(QLatin1String("Project.cbp"));
  QFile cbpFile(cbpPath.toString());
  if (!cbpFile.open(QFile::ReadOnly | QFile::Text))
  {
    qDebug() << "Error opening CodeBlocks Project File";
    return includePaths;
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
            if(!includePaths.contains(attribute_value))
            {
              includePaths.append(attribute_value);
            }
          }
        }
      }
    }
    cbpXml.readNext();
  }
  return includePaths;
}

QStringList ROSUtils::getROSPackages()
{
  QStringList output;
  ros::package::V_string packages;
  ros::package::getAll(packages);

  foreach(std::string str, packages)
  {
    output.append(QString::fromStdString(str));
  }

  return output;
}

QStringList ROSUtils::getROSPackageLaunchFiles(const QString &packageName, bool OnlyNames)
{
  QStringList launchFiles;
  if(!packageName.isEmpty())
  {
    QString path = QString::fromStdString(ros::package::getPath(packageName.toStdString()));
    const QDir srcDir(path);
    QDirIterator it(srcDir.absolutePath(),QStringList() << QLatin1String("*.launch"), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext())
    {
      QFileInfo launchFile(it.next());
      if(OnlyNames)
      {
        launchFiles.append(launchFile.baseName());
      }
      else
      {
        launchFiles.append(launchFile.absoluteFilePath());
      }
    }
  }

  return launchFiles;
}

} //namespace Internal
} //namespace ROSProjectManager
