#include "ros_utils.h"
#include "ros_project_constants.h"

#include <utils/fileutils.h>
#include <utils/environment.h>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QTextStream>

namespace ROSProjectManager {
namespace Internal {

ROSUtils::ROSUtils()
{

}

bool ROSUtils::generateCodeBlocksProjectFile(QProcess *process, const Utils::FileName &sourceDir, const Utils::FileName &buildDir)
{
    QString cmd = QLatin1String("cmake ") + sourceDir.toString() + QLatin1String(" -G \"CodeBlocks - Unix Makefiles\"");
    bool results = false;
    process->setWorkingDirectory(buildDir.toString());
    process->start(QLatin1String("bash"), QStringList() << QLatin1String("-c") << cmd);
    process->waitForFinished();
    if (process->exitStatus() != QProcess::CrashExit)
    {
      results  = true;
    }
    return results;
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

} //namespace Internal
} //namespace ROSProjectManager
