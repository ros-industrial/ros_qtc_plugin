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

#include "ros_project_nodes.h"
#include "ros_project.h"

#include <utils/fileutils.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

ROSProjectNode::ROSProjectNode(ROSProject *project)
    : ProjectNode(project->projectFilePath()), m_project(project)
{
    setDisplayName(project->projectFilePath().toFileInfo().completeBaseName());
}

void ROSProjectNode::diffWorkspaceFiles(const QHash<QString, QStringList> oldWorkspaceFiles,
                                        const QHash<QString, QStringList> newWorkspaceFiles,
                                        QHash<QString, QStringList> &addedFiles,
                                        QHash<QString, QStringList> &removedFiles)
{
  QSet<QString> oldDirs = oldWorkspaceFiles.keys().toSet();
  QSet<QString> newDirs = newWorkspaceFiles.keys().toSet();
  QSet<QString> oldFiles;
  QSet<QString> newFiles;

  QHashIterator<QString, QStringList> item(newWorkspaceFiles);
  while (item.hasNext())
  {
    item.next();
    // if they both contain the same directory check if files have been added or removed.
    if (oldWorkspaceFiles.contains(item.key()))
    {
      oldFiles = oldWorkspaceFiles[item.key()].toSet();
      newFiles = item.value().toSet();

      QSet<QString> removed = oldFiles;
      removed.subtract(newFiles);

      QSet<QString> added = newFiles;
      added.subtract(oldFiles);

      foreach (QString path, added)
      {
        addedFiles[item.key()].append(path);
      }

      foreach (QString path, removed)
      {
        removedFiles[item.key()].append(path);
      }
    }
    else
    {
      foreach (QString path, item.value())
      {
        addedFiles[item.key()].append(path);
      }
    }
  }

  //Now check is directory was removed
  QSet<QString> removedDirs = oldDirs;
  removedDirs.subtract(newDirs);
  foreach (QString dirPath, removedDirs)
  {
    removedFiles[dirPath].append(QLatin1String("REMOVE_FOLDER"));
  }
}

void ROSProjectNode::refresh(QHash<QString, QStringList> oldWorkspaceFiles)
{
    typedef QHash<QString, QStringList> FilesInPathHash;
    typedef FilesInPathHash::ConstIterator FilesInPathHashConstIt;

    FilesInPathHash newWorkspaceFiles = m_project->workspaceFiles();
    FilesInPathHash addedFiles, removedFiles;

    diffWorkspaceFiles(oldWorkspaceFiles, newWorkspaceFiles, addedFiles, removedFiles);


    QStringList sourceExtension, headerExtension;
    sourceExtension << QLatin1Literal("c")
                    << QLatin1Literal("cc")
                    << QLatin1Literal("cpp")
                    << QLatin1Literal("c++")
                    << QLatin1Literal("cp")
                    << QLatin1Literal("cxx");
    headerExtension << QLatin1Literal("h")
                    << QLatin1Literal("hh")
                    << QLatin1Literal("hpp")
                    << QLatin1Literal("h++")
                    << QLatin1Literal("hp")
                    << QLatin1Literal("hxx");

    QDir baseDir(m_project->projectDirectory().toString());
    FilesInPathHashConstIt cend = addedFiles.constEnd();
    for (FilesInPathHashConstIt it = addedFiles.constBegin(); it != cend; ++it) {
        const QString &filePath = baseDir.relativeFilePath(it.key());
        QStringList components;
        if (!filePath.isEmpty())
            components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findFolderByName(components, components.size());
        if (!folder)
            folder = createFolderByName(components, components.size());

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value())
        {
            if (file != QLatin1Literal("EMPTY_FOLDER"))
            {
              QFileInfo fileInfo(file);
              FileType fileType = ResourceType;
              if (headerExtension.contains(fileInfo.suffix()))
                fileType = HeaderType;
              else if(sourceExtension.contains(fileInfo.suffix()))
                fileType = SourceType;

              FileNode *fileNode = new FileNode(Utils::FileName::fromString(file),
                                                fileType, /*generated = */ false);
              fileNodes.append(fileNode);
            }
        }

        folder->addFileNodes(fileNodes);
    }

    cend = removedFiles.constEnd();
    for (FilesInPathHashConstIt it = removedFiles.constBegin(); it != cend; ++it) {
        const QString &filePath = baseDir.relativeFilePath(it.key());
        QStringList components;
        if (!filePath.isEmpty())
            components = filePath.split(QLatin1Char('/'));
        FolderNode *folder = findFolderByName(components, components.size());

        //Need to get parent folder also for empty folder removal
        FolderNode *parentFolder = NULL;
        if (components.size() >= 2)
        {
          components.removeLast();
          parentFolder = findFolderByName(components, components.size());
        }
        else
        {
          parentFolder = projectNode()->asFolderNode();
        }

        QList<FileNode *> fileNodes;
        foreach (const QString &file, it.value())
        {
          if (file != QLatin1Literal("REMOVE_FOLDER") && file != QLatin1Literal("EMPTY_FOLDER"))
          {
            foreach (FileNode *fn, folder->fileNodes()) {
                if (fn->filePath().toString() == file)
                    fileNodes.append(fn);
            }
            folder->removeFileNodes(fileNodes);
          }
          else if (file == QLatin1Literal("REMOVE_FOLDER"))
          {
            parentFolder->removeFolderNodes(QList<FolderNode *>() << folder);
          }
        } 
    }
}

FolderNode *ROSProjectNode::createFolderByName(const QStringList &components, int end)
{
    if (end == 0)
        return this;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    const QString component = components.at(end - 1);

    const Utils::FileName folderPath = filePath().parentDir().appendPath(folderName);
    FolderNode *folder = new FolderNode(folderPath);
    folder->setDisplayName(component);

    FolderNode *parent = findFolderByName(components, end - 1);
    if (!parent)
        parent = createFolderByName(components, end - 1);
    parent->addFolderNodes(QList<FolderNode*>() << folder);

    return folder;
}

FolderNode *ROSProjectNode::findFolderByName(const QStringList &components, int end)
{
    if (end == 0)
        return this;

    QString folderName;
    for (int i = 0; i < end; ++i) {
        folderName.append(components.at(i));
        folderName += QLatin1Char('/');
    }

    FolderNode *parent = findFolderByName(components, end - 1);

    if (!parent)
        return 0;

    const QString baseDir = filePath().toFileInfo().path();
    foreach (FolderNode *fn, parent->subFolderNodes()) {
        if (fn->filePath().toString() == baseDir + QLatin1Char('/') + folderName)
            return fn;
    }
    return 0;
}

bool ROSProjectNode::showInSimpleTree() const
{
    return true;
}

QList<ProjectAction> ROSProjectNode::supportedActions(Node *node) const
{
    Q_UNUSED(node);
    return QList<ProjectAction>()
        << AddNewFile
        << AddExistingFile
        << AddExistingDirectory
        << RemoveFile
        << Rename;
}

bool ROSProjectNode::addFiles(const QStringList &filePaths, QStringList *notAdded)
{
    Q_UNUSED(notAdded)

    return m_project->addFiles(filePaths);
}

bool ROSProjectNode::removeFiles(const QStringList &filePaths, QStringList *notRemoved)
{
    Q_UNUSED(notRemoved)

    return m_project->removeFiles(filePaths);
}

bool ROSProjectNode::renameFile(const QString &filePath, const QString &newFilePath)
{
    return m_project->renameFile(filePath, newFilePath);
}

} // namespace Internal
} // namespace ROSProjectManager
