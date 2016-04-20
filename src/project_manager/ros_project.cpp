#include "ros_project.h"

#include "ros_build_configuration.h"
#include "ros_make_step.h"
#include "ros_project_constants.h"
#include "ros_utils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <cpptools/cpptoolsconstants.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/projectpartbuilder.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/headerpath.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/customexecutablerunconfiguration.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProcessEnvironment>
#include <QtXml/QDomDocument>

using namespace Core;
using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

////////////////////////////////////////////////////////////////////////////////////
//
// ROSProject
//
////////////////////////////////////////////////////////////////////////////////////

ROSProject::ROSProject(ROSManager *manager, const QString &fileName)
{
    setId(Constants::ROSPROJECT_ID);
    setProjectManager(manager);
    setDocument(new ROSProjectFile(this, fileName));
    DocumentManager::addDocument(document(), true);
    setRootProjectNode(new ROSProjectNode(this->projectFilePath()));

    setProjectContext(Context(Constants::PROJECTCONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    m_projectName = projectFilePath().toFileInfo().completeBaseName();

    FileNode *projectWorkspaceNode = new FileNode(projectFilePath(),
                                                   ProjectFileType,
                                                   /* generated = */ false);

    rootProjectNode()->addFileNodes(QList<FileNode *>() << projectWorkspaceNode);

    projectManager()->registerProject(this);

    QObject::connect(&m_watcher, SIGNAL(directoryChanged(QString)),
                    this, SLOT(onDirectoryChanged(QString)));

    QObject::connect(this, &ROSProject::fileListChanged,
                     this, &ROSProject::refreshCppCodeModel);

}

ROSProject::~ROSProject()
{
    m_codeModelFuture.cancel();
    projectManager()->unregisterProject(this);
}

void ROSProject::addDirectory(const QString &parentPath, const QString &dirName)
{

  Utils::FileName directory = Utils::FileName::fromString(QString::fromLatin1("%1/%2").arg(parentPath, dirName));
  QStringList files;
  QStringList subDirectories;
  QHash<QString, ROSUtils::FolderContent> newDirContent = ROSUtils::getFolderContent(directory, files);
  QHashIterator<QString, ROSUtils::FolderContent> item(newDirContent);
  while (item.hasNext())
  {
    item.next();
    subDirectories.append(item.key());

    static_cast<ROSProjectNode *>(rootProjectNode())->addDirectory(item.key());
    foreach (QString file, item.value().files)
    {
      static_cast<ROSProjectNode *>(rootProjectNode())->addFile(item.key(), file);
    }
  }
  m_watcher.addPaths(subDirectories);
  m_workspaceContent.unite(newDirContent);
  m_workspaceFiles.append(files);
  emit fileListChanged();
}

void ROSProject::removeDirectory(const QString &parentPath, const QString &dirName)
{
  static_cast<ROSProjectNode *>(rootProjectNode())->removeDirectory(parentPath, dirName);

  Utils::FileName directory = Utils::FileName::fromString(QString::fromLatin1("%1/%2").arg(parentPath, dirName));
  QHashIterator<QString, ROSUtils::FolderContent> item(m_workspaceContent);
  while (item.hasNext())
  {
    item.next();
    if(item.key() == directory.toString() || item.key().startsWith(QString::fromLatin1("%1/").arg(directory.toString())))
    {
      foreach (QString file, item.value().files)
        m_workspaceFiles.removeAll(QString::fromLatin1("%1/%2").arg(item.key(), file));

      m_watcher.removePath(item.key());
      m_workspaceContent.remove(item.key());
    }
  }
  emit fileListChanged();
}

void ROSProject::renameDirectory(const QString &parentPath, const QString &oldDirName, const QString &newDirName)
{
  static_cast<ROSProjectNode *>(rootProjectNode())->renameDirectory(parentPath, oldDirName, newDirName);

  QString oldDirectory = QString::fromLatin1("%1/%2").arg(parentPath, oldDirName);
  QString newDirectory = QString::fromLatin1("%1/%2").arg(parentPath, newDirName);

  QHash<QString, ROSUtils::FolderContent> renamedContent;
  QHashIterator<QString, ROSUtils::FolderContent> item(m_workspaceContent);
  while (item.hasNext())
  {
    item.next();
    QString key = item.key();
    if(key == oldDirectory || key.startsWith(QString::fromLatin1("%1/").arg(oldDirectory)))
    {
      ROSUtils::FolderContent swapValues = item.value();
      QString newKey = key;

      m_watcher.removePath(key);
      m_workspaceContent.remove(key);

      newKey.replace(oldDirectory, newDirectory);
      renamedContent[newKey] = swapValues;

      m_watcher.addPath(newKey);
    }
    m_workspaceFiles.replaceInStrings(QString::fromLatin1("%1/").arg(oldDirectory), QString::fromLatin1("%1/").arg(newDirectory));
  }
  m_workspaceContent.unite(renamedContent);
  emit fileListChanged();
}

void ROSProject::onDirectoryChanged(const QString &path)
{
  // Compare the latest contents to saved contents for the dir updated to find out the difference(change)
  const QDir dir(path);
  QStringList curFiles = m_workspaceContent[path].files;
  QStringList curDirectories = m_workspaceContent[path].directories;

  QStringList newFiles = dir.entryList(QDir::NoDotAndDotDot  | QDir::Files);
  QStringList newDirectories = dir.entryList(QDir::NoDotAndDotDot  | QDir::Dirs);

  QSet<QString> curFileSet = curFiles.toSet();
  QSet<QString> newFileSet = newFiles.toSet();
  QSet<QString> curDirectorySet = curDirectories.toSet();
  QSet<QString> newDirectorySet = newDirectories.toSet();

  // Content that has been added
  QStringList addedFiles = (newFileSet - curFileSet).toList();
  QStringList addedDirectories = (newDirectorySet - curDirectorySet).toList();

  // Content that has been removed
  QStringList deletedFiles = (curFileSet - newFileSet).toList();
  QStringList deletedDirectories = (curDirectorySet - newDirectorySet).toList();

  //Handle Files
  if(!addedFiles.isEmpty() || !deletedFiles.isEmpty())
  {
    // File is renamed
    if(addedFiles.count() == 1 && deletedFiles.count() == 1)
    {
      //File Renamed
      static_cast<ROSProjectNode *>(rootProjectNode())->renameFile(path, deletedFiles.first(), addedFiles.first());
      QString temp = QString::fromLatin1("%1/%2").arg(path, deletedFiles.first());
      m_workspaceFiles.removeAll(QString::fromLatin1("%1/%2").arg(path, deletedFiles.first()));
      m_workspaceFiles.append(QString::fromLatin1("%1/%2").arg(path, addedFiles.first()));
    }
    else
    {
      // New File Added to Dir
      if(!addedFiles.isEmpty())
      {
        foreach(QString file, addedFiles)
        {
          static_cast<ROSProjectNode *>(rootProjectNode())->addFile(path, file);
          m_workspaceFiles.append(dir.absoluteFilePath(file));
        }
      }

      // File is deleted from Dir
      if(!deletedFiles.isEmpty())
      {
        foreach(QString file, deletedFiles)
        {
          static_cast<ROSProjectNode *>(rootProjectNode())->removeFile(path, file);
          m_workspaceFiles.removeAll(QString::fromLatin1("%1/%2").arg(path, file));
        }
      }
    }
    emit fileListChanged();
  }

  //Handle Directories
  if(!addedDirectories.isEmpty() || !deletedDirectories.isEmpty())
  {
    if(addedDirectories.count() == 1 && deletedDirectories.count() == 1)
    {
      // Directory is renamed
      renameDirectory(path, deletedDirectories.first(), addedDirectories.first());
    }
    else
    {
      // New Directory Added to Dir
      foreach(QString directory, addedDirectories)
      {
        addDirectory(path, directory);
      }

      // Directory is deleted from Dir
      foreach(QString directory, deletedDirectories)
      {
        //Directory deleted
        removeDirectory(path, directory);
      }
    }
  }

  // Update the current set
  m_workspaceContent[path].files = newFiles;
  m_workspaceContent[path].directories = newDirectories;

  //print();
}

Utils::FileName ROSProject::buildDirectory() const
{
  return projectDirectory().appendPath(tr("build"));
}

Utils::FileName ROSProject::sourceDirectory() const
{
  return projectDirectory().appendPath(tr("src"));
}

bool ROSProject::saveProjectFile()
{
    DocumentManager::expectFileChange(projectFilePath().toString());
    // Make sure we can open the file for writing

    Utils::FileSaver saver(projectFilePath().toString(), QIODevice::Text);
    if (!saver.hasError())
    {
      QXmlStreamWriter workspaceXml(saver.file());
      ROSUtils::gererateQtCreatorWorkspaceFile(workspaceXml, m_watchDirectories, m_projectIncludePaths);
      saver.setResult(&workspaceXml);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(projectFilePath().toString());
    return result;
}

bool ROSProject::addIncludes(const QStringList &includePaths)
{
    foreach (const QString &includePath, includePaths)
    {
      if (QDir(includePath).exists())
        m_projectIncludePaths.append(includePath);
    }

    m_projectIncludePaths.removeDuplicates();
    bool result = saveProjectFile();
    refresh();

    return result;
}

bool ROSProject::setIncludes(const QStringList &includePaths)
{
    m_projectIncludePaths.clear();
    bool result = addIncludes(includePaths);
    return result;
}

void ROSProject::parseProjectFile()
{
    QXmlStreamReader workspaceXml;
    QFile workspaceFile(projectFilePath().toString());
    QDir baseDir(projectDirectory().toString());
    if (workspaceFile.open(QFile::ReadOnly | QFile::Text))
    {
      m_watchDirectories.clear();
      m_projectIncludePaths.clear();

      workspaceXml.setDevice(&workspaceFile);
      while(workspaceXml.readNextStartElement())
      {
        if(workspaceXml.name() == QLatin1String("WatchDirectories"))
        {
          while(workspaceXml.readNextStartElement())
          {
            if(workspaceXml.name() == QLatin1String("Directory"))
            {
              m_watchDirectories.append(workspaceXml.readElementText());
            }
          }
        }
        else if(workspaceXml.name() == QLatin1String("IncludePaths"))
        {
          while(workspaceXml.readNextStartElement())
          {
            if(workspaceXml.name() == QLatin1String("Directory"))
            {
              m_projectIncludePaths.append(workspaceXml.readElementText());
            }
          }
        }
      }
    }
    else
    {
      qDebug() << "Error opening Workspace Project File";
    }
}

void ROSProject::refresh()
{
    QSet<QString> oldWatchDirectories = m_watchDirectories.toSet();
    parseProjectFile();
    QSet<QString> newWatchDirectories = m_watchDirectories.toSet();

    QStringList addedDirectories = (newWatchDirectories - oldWatchDirectories).toList();
    QStringList removedDirectories = (oldWatchDirectories - newWatchDirectories).toList();

    foreach (QString dir, removedDirectories) {
      Utils::FileName removedDir = projectDirectory().appendPath(dir);

      if (removedDir.isChildOf(projectDirectory()))
        removeDirectory(removedDir.parentDir().toString(), dir);
    }

    foreach (QString dir, addedDirectories) {
      Utils::FileName addedDir = projectDirectory().appendPath(dir);
      if (addedDir.isChildOf(projectDirectory()) && addedDir.exists())
        addDirectory(addedDir.parentDir().toString(), dir);
    }

    if (addedDirectories.empty() && removedDirectories.empty())
      refreshCppCodeModel();

    //print();
}

void ROSProject::refreshCppCodeModel()
{
    CppTools::CppModelManager *modelManager = CppTools::CppModelManager::instance();

    m_codeModelFuture.cancel();

    CppTools::ProjectInfo pInfo(this);
    CppTools::ProjectPartBuilder ppBuilder(pInfo);

    CppTools::ProjectPart::QtVersion activeQtVersion = CppTools::ProjectPart::NoQt;
    if (QtSupport::BaseQtVersion *qtVersion =
            QtSupport::QtKitInformation::qtVersion(activeTarget()->kit())) {
        if (qtVersion->qtVersion() < QtSupport::QtVersionNumber(5,0,0))
            activeQtVersion = CppTools::ProjectPart::Qt4;
        else
            activeQtVersion = CppTools::ProjectPart::Qt5;
    }

    ppBuilder.setQtVersion(activeQtVersion);
    ppBuilder.setIncludePaths(projectIncludePaths());
    ppBuilder.setCxxFlags(QStringList() << QLatin1String("-std=c++11"));

    const QList<Id> languages = ppBuilder.createProjectPartsForFiles(m_workspaceFiles);
    foreach (Id language, languages)
        setProjectLanguage(language, true);

    pInfo.finish();
    m_codeModelFuture = modelManager->updateProjectInfo(pInfo);
}

QStringList ROSProject::projectIncludePaths() const
{
    return m_projectIncludePaths;
}

QStringList ROSProject::workspaceFiles() const
{
    return m_workspaceFiles;
}

QString ROSProject::displayName() const
{
    return m_projectName;
}

QStringList ROSProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_workspaceFiles;
}

ROSManager *ROSProject::projectManager() const
{
  return static_cast<ROSManager *>(Project::projectManager());
}

QStringList ROSProject::buildTargets() const
{
    QStringList targets;
    targets.append(QLatin1String("all"));
    targets.append(QLatin1String("clean"));
    return targets;
}

Project::RestoreResult ROSProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
      RestoreResult result = Project::fromMap(map, errorMessage);
      if (result != RestoreResult::Ok)
          return result;

      Kit *defaultKit = KitManager::defaultKit();
      if (!activeTarget() && defaultKit)
          addTarget(createTarget(defaultKit));

      // Sanity check: We need both a buildconfiguration and a runconfiguration!
      QList<Target *> targetList = targets();
      if (targetList.isEmpty())
          return RestoreResult::Error;

      foreach (Target *t, targetList) {
          if (!t->activeBuildConfiguration()) {
              removeTarget(t);
              continue;
          }
          if (!t->activeRunConfiguration())
              t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));
      }

      refresh();
      return RestoreResult::Ok;
}

void ROSProject::print()
{
  QHashIterator<QString, ROSUtils::FolderContent> item(m_workspaceContent);
  qDebug() << "Workspace Content:";
  while (item.hasNext())
  {
    item.next();
    qDebug() << "Parent:" << item.key();
    qDebug() << "  Files: ";
    foreach (QString str, item.value().files)
      qDebug() << "    " << str;

    qDebug() << "  SubDirectories: ";
    foreach (QString str, item.value().directories)
      qDebug() << "    " << str;
  }

  qDebug() << "File List:";
  foreach (QString str, m_workspaceFiles)
    qDebug() << "  " << str;

}

////////////////////////////////////////////////////////////////////////////////////
//
// ROSProjectFile
//
////////////////////////////////////////////////////////////////////////////////////

ROSProjectFile::ROSProjectFile(ROSProject *parent, QString fileName)
    : IDocument(parent),
      m_project(parent)
{
    setId("ROS.ProjectFile");
    setMimeType(QLatin1String(Constants::ROSMIMETYPE));
    setFilePath(Utils::FileName::fromString(fileName));
}

bool ROSProjectFile::save(QString *, const QString &, bool)
{
    return false;
}

bool ROSProjectFile::isModified() const
{
    return false;
}

bool ROSProjectFile::isSaveAsAllowed() const
{
    return false;
}

IDocument::ReloadBehavior ROSProjectFile::reloadBehavior(ChangeTrigger state, ChangeType type) const
{
    Q_UNUSED(state)
    Q_UNUSED(type)
    return BehaviorSilent;
}

bool ROSProjectFile::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(errorString)
    Q_UNUSED(flag)
    if (type == TypePermissions)
        return true;
    m_project->refresh();
    return true;
}

} // namespace Internal
} // namespace GenericProjectManager
