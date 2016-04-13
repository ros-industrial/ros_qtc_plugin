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
    setRootProjectNode(new ROSProjectNode(this));

    setProjectContext(Context(Constants::PROJECTCONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    m_projectName = projectFilePath().toFileInfo().completeBaseName();

    FileNode *projectWorkspaceNode = new FileNode(projectFilePath(),
                                                   ProjectFileType,
                                                   /* generated = */ false);

    rootProjectNode()->addFileNodes(QList<FileNode *>() << projectWorkspaceNode);

    projectManager()->registerProject(this);
}

ROSProject::~ROSProject()
{
    m_codeModelFuture.cancel();
    projectManager()->unregisterProject(this);
}

Utils::FileName ROSProject::buildDirectory() const
{
  return projectDirectory().appendPath(tr("build"));
}

Utils::FileName ROSProject::sourceDirectory() const
{
  return projectDirectory().appendPath(tr("src"));
}

bool ROSProject::saveWorkspaceFiles(const QHash<QString, QStringList> &workspaceFiles)
{
    DocumentManager::expectFileChange(projectFilePath().toString());
    // Make sure we can open the file for writing

    Utils::FileSaver saver(projectFilePath().toString(), QIODevice::Text);
    if (!saver.hasError())
    {
      QXmlStreamWriter workspaceXml(saver.file());
      ROSUtils::gererateQtCreatorWorkspaceFile(workspaceXml, workspaceFiles, m_projectIncludePaths);
      saver.setResult(&workspaceXml);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(projectFilePath().toString());
    return result;
}

bool ROSProject::saveProjectIncludePaths()
{
    DocumentManager::expectFileChange(projectFilePath().toString());
    // Make sure we can open the file for writing

    Utils::FileSaver saver(projectFilePath().toString(), QIODevice::Text);
    if (!saver.hasError())
    {
      QXmlStreamWriter workspaceXml(saver.file());
      ROSUtils::gererateQtCreatorWorkspaceFile(workspaceXml, m_workspaceFiles, m_projectIncludePaths);
      saver.setResult(&workspaceXml);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(projectFilePath().toString());
    return result;
}

bool ROSProject::addFiles(const QStringList &filePaths)
{
    QHash<QString, QStringList> newHash = m_workspaceFiles;

    QDir baseDir(projectDirectory().toString());
    foreach (const QString &filePath, filePaths)
    {
        QFileInfo fileInfo(baseDir.absoluteFilePath(filePath));
        newHash[fileInfo.absoluteDir().absolutePath()].append(fileInfo.absoluteFilePath());
        newHash[fileInfo.absoluteDir().absolutePath()].removeDuplicates();
        newHash[fileInfo.absoluteDir().absolutePath()].removeAll(QLatin1String("EMPTY_FOLDER"));
    }

    bool result = saveWorkspaceFiles(newHash);
    refresh();

    return result;
}

bool ROSProject::removeFiles(const QStringList &filePaths)
{
    QHash<QString, QStringList> newHash = m_workspaceFiles;

    QDir baseDir(projectDirectory().toString());
    foreach (const QString &filePath, filePaths)
    {
        QFileInfo fileInfo(baseDir.absoluteFilePath(filePath));
        newHash[fileInfo.absoluteDir().absolutePath()].removeAll(fileInfo.absoluteFilePath());
        if (newHash[fileInfo.absoluteDir().absolutePath()].isEmpty())
          newHash[fileInfo.absoluteDir().absolutePath()].append(QLatin1String("EMPTY_FOLDER"));
    }

    bool result = saveWorkspaceFiles(newHash);
    refresh();

    return result;
}

bool ROSProject::renameFile(const QString &filePath, const QString &newFilePath)
{
    QHash<QString, QStringList> newHash = m_workspaceFiles;

    QDir baseDir(projectDirectory().toString());

    QFileInfo fileInfo(baseDir.absoluteFilePath(filePath));
    if (newHash[fileInfo.absoluteDir().absolutePath()].contains(baseDir.absoluteFilePath(filePath)))
    {
      int idx = newHash[fileInfo.absoluteDir().absolutePath()].indexOf(baseDir.absoluteFilePath(filePath));
      newHash[fileInfo.absoluteDir().absolutePath()].replace(idx, baseDir.absoluteFilePath(newFilePath));
    }


    bool result = saveWorkspaceFiles(newHash);
    refresh();

    return result;
}

bool ROSProject::setWorkspaceFiles(const QHash<QString, QStringList> &workspaceFiles)
{
    QHash<QString, QStringList> newHash;
    QDir baseDir(projectDirectory().toString());

    QHashIterator<QString, QStringList> item(workspaceFiles);
    while (item.hasNext())
    {
      item.next();
      QString absoluteDirPath = baseDir.absoluteFilePath(item.key());

      if (item.value().count() == 1 && item.value().at(0) == QLatin1String("EMPTY_FOLDER"))
      {
        newHash[absoluteDirPath].append(QLatin1String("EMPTY_FOLDER"));
      }
      else if (item.value().count() > 0 )
      {
        foreach (QString var, item.value())
        {
           newHash[absoluteDirPath].append(baseDir.absoluteFilePath(var));
        }
      }
      else
      {
        qDebug() << "Error in ROSProject::setWorkspaceFiles";
      }
    }

    bool result = saveWorkspaceFiles(newHash);
    refresh();

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
    bool result = saveProjectIncludePaths();
    refresh();

    return result;
}

bool ROSProject::setIncludes(const QStringList &includePaths)
{
    m_projectIncludePaths.clear();
    bool result = addIncludes(includePaths);
    return result;
}


void ROSProject::parseProject()
{
    QXmlStreamReader workspaceXml;
    QFile workspaceFile(projectFilePath().toString());
    QDir baseDir(projectDirectory().toString());
    if (workspaceFile.open(QFile::ReadOnly | QFile::Text))
    {
      m_workspaceFiles.clear();
      m_projectIncludePaths.clear();
      m_workspaceFileList.clear();

      workspaceXml.setDevice(&workspaceFile);
      while(workspaceXml.readNextStartElement())
      {
        if(workspaceXml.name() == QLatin1String("Files"))
        {
          while(workspaceXml.readNextStartElement())
          {
            if (workspaceXml.name() == QLatin1String("Directory"))
            {
              QString dirPath;
              QString empty;
              foreach(const QXmlStreamAttribute &attr, workspaceXml.attributes())
              {
                if (attr.name().toString() == QLatin1String("path"))
                {
                    dirPath = attr.value().toString();
                }
                else if (attr.name().toString() == QLatin1String("empty"))
                {
                    empty = attr.value().toString();
                }
              }

              if (dirPath.isEmpty() || empty.isEmpty())
              {
                qDebug() << "Error parsing .workspace file missing attributes path and empty.";
              }
              else
              {
                QDir curDir(baseDir.absoluteFilePath(dirPath));
                if (curDir.exists())
                {
                  if (empty.toLower() == QLatin1String("true"))
                  {
                    m_workspaceFiles[curDir.absolutePath()].append(QLatin1String("EMPTY_FOLDER"));
                    workspaceXml.skipCurrentElement();
                  }
                  else
                  {
                    while(workspaceXml.readNextStartElement())
                    {
                      if (workspaceXml.name() == QLatin1String("File"))
                      {
                        QString absoluteFilePath = curDir.absoluteFilePath(workspaceXml.readElementText());
                        m_workspaceFiles[curDir.absolutePath()].append(absoluteFilePath);
                        m_workspaceFileList.append(absoluteFilePath);
                      }
                      else
                      {
                        workspaceXml.skipCurrentElement();
                      }
                    }
                  }
                }
                else
                {
                  qDebug() << "The project file contained a directory that does not exist:" << curDir.absolutePath();
                }
              }
            }
            else
            {
              workspaceXml.skipCurrentElement();
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
    emit fileListChanged();

}

void ROSProject::refresh()
{
    QHash<QString, QStringList> oldWorkspaceFiles;
    oldWorkspaceFiles = m_workspaceFiles;

    parseProject();

    static_cast<ROSProjectNode *>(rootProjectNode())->refresh(oldWorkspaceFiles);

    refreshCppCodeModel();
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

    const QList<Id> languages = ppBuilder.createProjectPartsForFiles(m_workspaceFileList);
    foreach (Id language, languages)
        setProjectLanguage(language, true);

    pInfo.finish();
    m_codeModelFuture = modelManager->updateProjectInfo(pInfo);
}

QStringList ROSProject::projectIncludePaths() const
{
    return m_projectIncludePaths;
}

QHash<QString, QStringList> ROSProject::workspaceFiles() const
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
    return m_workspaceFileList;
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
