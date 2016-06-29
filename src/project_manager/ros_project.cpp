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
#include "ros_project.h"

#include "ros_build_configuration.h"
#include "ros_make_step.h"
#include "ros_project_constants.h"
#include "ros_utils.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
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
  : m_workspaceWatcher(new ROSWorkspaceWatcher(this))
{
    setId(Constants::ROSPROJECT_ID);
    setProjectManager(manager);
    setDocument(new ROSProjectFile(this, fileName));
    DocumentManager::addDocument(document(), true);

    ROSProjectNode *project_node = new ROSProjectNode(this->projectFilePath());
    connect(Core::VcsManager::instance(), &Core::VcsManager::repositoryChanged,
            this, &ROSProject::repositoryChanged);
    setRootProjectNode(project_node);

    setProjectContext(Context(Constants::PROJECTCONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    m_projectName = projectFilePath().toFileInfo().completeBaseName();

    FileNode *projectWorkspaceNode = new FileNode(projectFilePath(),
                                                   ProjectFileType,
                                                   /* generated = */ false);

    rootProjectNode()->addFileNodes(QList<FileNode *>() << projectWorkspaceNode);

    projectManager()->registerProject(this);

    connect(m_workspaceWatcher, SIGNAL(fileListChanged()),
            this, SIGNAL(fileListChanged()));

    connect(this, &ROSProject::fileListChanged,
            this, &ROSProject::refresh);

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
        m_workspaceWatcher->unwatchFolder(removedDir.parentDir().toString(), dir);
    }

    foreach (QString dir, addedDirectories) {
      Utils::FileName addedDir = projectDirectory().appendPath(dir);
      if (addedDir.isChildOf(projectDirectory()) && addedDir.exists())
        m_workspaceWatcher->watchFolder(addedDir.parentDir().toString(), dir);
    }

    // This will occure when include directories are added.
    if (addedDirectories.isEmpty() && removedDirectories.isEmpty())
      refreshCppCodeModel();

//    m_workspaceWatcher->print();
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

    const QList<Id> languages = ppBuilder.createProjectPartsForFiles(m_workspaceWatcher->getWorkspaceFiles());
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
    return m_workspaceWatcher->getWorkspaceFiles();
}

QString ROSProject::displayName() const
{
    return m_projectName;
}

QStringList ROSProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_workspaceWatcher->getWorkspaceFiles();
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

void ROSProject::repositoryChanged(const QString &repository)
{
  static_cast<ROSProjectNode *>(rootProjectNode())->updateVersionControlInfo(repository);
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
