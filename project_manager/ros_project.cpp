/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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

ROSProject::ROSProject(Manager *manager, const QString &fileName)
    : m_manager(manager),
      m_fileName(fileName)
{
    setId(Constants::ROSPROJECT_ID);
    setProjectContext(Context(ROSProjectManager::Constants::PROJECTCONTEXT));
    setProjectLanguages(Context(ProjectExplorer::Constants::LANG_CXX));

    QFileInfo fileInfo(m_fileName);
    QDir dir = fileInfo.dir();

    m_projectName       = fileInfo.completeBaseName();
    m_workspaceFileName = QFileInfo(dir, m_projectName + QLatin1String(".workspace")).absoluteFilePath();

    m_creatorIDocument  = new ROSProjectFile(this, m_fileName);
    m_workspaceIDocument = new ROSProjectFile(this, m_workspaceFileName);

    DocumentManager::addDocument(m_creatorIDocument);
    DocumentManager::addDocument(m_workspaceIDocument);

    m_rootNode = new ROSProjectNode(this, m_creatorIDocument);

    FileNode *projectWorkspaceFileNode = new FileNode(Utils::FileName::fromString(m_workspaceFileName),
                                                      ProjectFileType,
                                                      /* generated = */ false);


    m_rootNode->addFileNodes(QList<FileNode *>()
                             << projectWorkspaceFileNode);

    m_manager->registerProject(this);
}

ROSProject::~ROSProject()
{
    m_codeModelFuture.cancel();
    m_manager->unregisterProject(this);

    delete m_rootNode;
}

QString ROSProject::workspaceFileName() const
{
    return m_workspaceFileName;
}

bool ROSProject::saveRawList(const QStringList &rawList, const ROSProject::UpdateOptions &updateOption)
{
    DocumentManager::expectFileChange(m_workspaceFileName);
    // Make sure we can open the file for writing

    QStringList files, includePaths;

    if (updateOption == UpdateOptions::Files)
    {
      files = rawList;
      includePaths = m_projectIncludePaths;
    }
    else if (updateOption == UpdateOptions::IncludePaths)
    {
      files = m_rawFileList;
      includePaths = rawList;
    }

    Utils::FileSaver saver(m_workspaceFileName, QIODevice::Text);
    if (!saver.hasError())
    {
      QXmlStreamWriter workspaceXml(saver.file());
      ROSUtils::gererateQtCreatorWorkspaceFile(workspaceXml, files, includePaths);
      saver.setResult(&workspaceXml);
    }
    bool result = saver.finalize(ICore::mainWindow());
    DocumentManager::unexpectFileChange(m_workspaceFileName);
    return result;
}

bool ROSProject::addFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));


    QSet<QString> includes = projectIncludePaths().toSet();
    QSet<QString> toAdd;

    foreach (const QString &filePath, filePaths) {
        QString directory = QFileInfo(filePath).absolutePath();
        if (!includes.contains(directory)
                && !toAdd.contains(directory))
            toAdd << directory;
    }

    const QDir dir(projectDirectory().toString());
    foreach (const QString &path, toAdd) {
        QString relative = dir.relativeFilePath(path);
        if (relative.isEmpty())
            relative = QLatin1Char('.');
        m_rawProjectIncludePaths.append(relative);
    }

    bool result = saveRawList(newList, UpdateOptions::Files);
    result &= saveRawList(m_rawProjectIncludePaths, UpdateOptions::IncludePaths);
    refresh();

    return result;
}

bool ROSProject::removeFiles(const QStringList &filePaths)
{
    QStringList newList = m_rawFileList;

    foreach (const QString &filePath, filePaths) {
        QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    return saveRawList(newList, UpdateOptions::Files);
}

bool ROSProject::setFiles(const QStringList &filePaths)
{
    QStringList newList;
    QDir baseDir(QFileInfo(m_fileName).dir());
    foreach (const QString &filePath, filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawList(newList, UpdateOptions::Files);
}

bool ROSProject::renameFile(const QString &filePath, const QString &newFilePath)
{
    QStringList newList = m_rawFileList;

    QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
    if (i != m_rawListEntries.end()) {
        int index = newList.indexOf(i.value());
        if (index != -1) {
            QDir baseDir(QFileInfo(m_fileName).dir());
            newList.replace(index, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawList(newList, UpdateOptions::Files);
}

void ROSProject::parseProject()
{
    QXmlStreamReader workspaceXml;
    QFile workspaceFile(m_workspaceFileName);
    if (workspaceFile.open(QFile::ReadOnly | QFile::Text))
    {
      m_rawListEntries.clear();
      m_rawFileList.clear();
      m_rawProjectIncludePaths.clear();

      workspaceXml.setDevice(&workspaceFile);
      workspaceXml.readNext();
      while(!workspaceXml.atEnd())
      {
        if(workspaceXml.isStartElement())
        {
          if(workspaceXml.name() == QLatin1String("File"))
          {
            m_rawFileList.append(workspaceXml.readElementText());

          }
          else if(workspaceXml.name() == QLatin1String("Directory"))
          {
            m_rawProjectIncludePaths.append(workspaceXml.readElementText());

          }
        }
        workspaceXml.readNext();
      }
      m_files = processEntries(m_rawFileList, &m_rawListEntries);
      m_projectIncludePaths = processEntries(m_rawProjectIncludePaths);
    }
    else
    {
      qDebug() << "Error opening Workspace Project File";
    }
    emit fileListChanged();

}

void ROSProject::refresh()
{
    QSet<QString> oldFileList;
    oldFileList = m_files.toSet();
    parseProject();

    m_rootNode->refresh(oldFileList);
    refreshCppCodeModel();
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    static QRegExp candidate(QLatin1String("\\$\\$\\((.+)\\)"));

    int index = candidate.indexIn(string);
    while (index != -1) {
        const QString value = env.value(candidate.cap(1));

        string.replace(index, candidate.matchedLength(), value);
        index += value.length();

        index = candidate.indexIn(string, index);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a entries.
 */
QStringList ROSProject::processEntries(const QStringList &paths,
                                           QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(QFileInfo(m_fileName).dir());

    QFileInfo fileInfo;
    QStringList absolutePaths;
    foreach (const QString &path, paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        trimmedPath = Utils::FileName::fromUserInput(trimmedPath).toString();

        fileInfo.setFile(projectDir, trimmedPath);
        if (fileInfo.exists()) {
            const QString absPath = fileInfo.absoluteFilePath();
            absolutePaths.append(absPath);
            if (map)
                map->insert(absPath, trimmedPath);
        }
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
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
    //ppBuilder.setConfigFileName(configFileName());
    ppBuilder.setCxxFlags(QStringList() << QLatin1String("-std=c++11"));

    const QList<Id> languages = ppBuilder.createProjectPartsForFiles(files());
    foreach (Id language, languages)
        setProjectLanguage(language, true);

    pInfo.finish();
    m_codeModelFuture = modelManager->updateProjectInfo(pInfo);
}

QStringList ROSProject::projectIncludePaths() const
{
    return m_projectIncludePaths;
}

QStringList ROSProject::files() const
{
    return m_files;
}

QString ROSProject::displayName() const
{
    return m_projectName;
}

IDocument *ROSProject::document() const
{
    return m_creatorIDocument;
}

IProjectManager *ROSProject::projectManager() const
{
    return m_manager;
}

ROSProjectNode *ROSProject::rootProjectNode() const
{
    return m_rootNode;
}

QStringList ROSProject::files(FilesMode fileMode) const
{
    Q_UNUSED(fileMode)
    return m_files;
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

QString ROSProjectFile::defaultPath() const
{
    return QString();
}

QString ROSProjectFile::suggestedFileName() const
{
    return QString();
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
