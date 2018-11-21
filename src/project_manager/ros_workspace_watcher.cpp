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
#include "ros_workspace_watcher.h"
#include "ros_project_constants.h"
#include "ros_project.h"

#include <projectexplorer/projecttree.h>

#include <QDebug>

namespace ROSProjectManager {
namespace Internal {

ROSWorkspaceWatcher::ROSWorkspaceWatcher(ROSProject *parent)
  :QObject(parent), m_project(parent)
{
  connect(&m_watcher, SIGNAL(directoryChanged(QString)),
          this, SLOT(onFolderChanged(QString)));
}

void ROSWorkspaceWatcher::watchFolder(const QString &parentPath, const QString &dirName)
{

  Utils::FileName directory = Utils::FileName::fromString(parentPath).appendPath(dirName);
  QStringList files;
  QStringList subDirectories;
  QHash<QString, ROSUtils::FolderContent> newDirContent = ROSUtils::getFolderContent(directory, files);

  // This to handle when watching workspace directory
  if (directory == m_project->projectDirectory())
  {
      files.removeAll(m_project->projectFilePath().toString());
      newDirContent[directory.toString()].removeFiles(QStringList() << m_project->projectFilePath().fileName());
  }

  QHashIterator<QString, ROSUtils::FolderContent> item(newDirContent);
  while (item.hasNext())
  {
    item.next();

    subDirectories.append(item.key());

    // Add directory node
    static_cast<ROSProjectNode *>(m_project->rootProjectNode())->addDirectory(item.key());

    // Add all files in the directory node
    for (const QString& file : item.value().files)
    {
      static_cast<ROSProjectNode *>(m_project->rootProjectNode())->addFile(item.key(), file);
    }
  }
  m_watcher.addPaths(subDirectories);
  m_workspaceContent.unite(newDirContent);
  m_workspaceFiles.append(files);
  ProjectExplorer::ProjectTree::emitSubtreeChanged(m_project->rootProjectNode());
  emit fileListChanged();
}

void ROSWorkspaceWatcher::unwatchFolder(const QString &parentPath, const QString &dirName)
{
  Utils::FileName directory = Utils::FileName::fromString(parentPath).appendPath(dirName);

  // This is to handle when watching workspace directory.
  if (!(directory == m_project->projectDirectory()))
  {
        static_cast<ROSProjectNode *>(m_project->rootProjectNode())->removeDirectory(parentPath, dirName);
  }
  else
  {
      ROSProjectNode *folder = static_cast<ROSProjectNode *>(m_project->rootProjectNode());
      for (ProjectExplorer::Node *n : folder->nodes())
          if (m_project->projectFilePath() != n->filePath())
            folder->takeNode(n);
  }

  QHashIterator<QString, ROSUtils::FolderContent> item(m_workspaceContent);
  while (item.hasNext())
  {
    item.next();
    if(item.key() == directory.toString() || item.key().startsWith(QString::fromLatin1("%1/").arg(directory.toString())))
    {
      for (const QString& file : item.value().files)
        m_workspaceFiles.removeAll(QString::fromLatin1("%1/%2").arg(item.key(), file));

      m_watcher.removePath(item.key());
      m_workspaceContent.remove(item.key());
    }
  }
  ProjectExplorer::ProjectTree::emitSubtreeChanged(m_project->rootProjectNode());
  emit fileListChanged();
}

void ROSWorkspaceWatcher::renameFolder(const QString &parentPath, const QString &oldDirName, const QString &newDirName)
{
  static_cast<ROSProjectNode *>(m_project->rootProjectNode())->renameDirectory(parentPath, oldDirName, newDirName);

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

void ROSWorkspaceWatcher::onFolderChanged(const QString &path)
{
  // Compare the latest contents to saved contents for the dir updated to find out the difference(change)
  const QDir dir(path);
  QStringList curFiles = m_workspaceContent[path].files;
  QStringList curDirectories = m_workspaceContent[path].directories;

  QStringList newFiles = dir.entryList(QDir::NoDotAndDotDot  | QDir::Files | QDir::Hidden);
  QStringList newDirectories = dir.entryList(QDir::NoDotAndDotDot  | QDir::Dirs);
  QStringList excludeFiles = dir.entryList(Constants::ROS_EXCLUDE_FILE_EXTENSION, QDir::NoDotAndDotDot  | QDir::Files | QDir::Hidden);

  QSet<QString> curFileSet = curFiles.toSet();
  QSet<QString> newFileSet = newFiles.toSet() - excludeFiles.toSet();
  QSet<QString> curDirectorySet = curDirectories.toSet();
  QSet<QString> newDirectorySet = newDirectories.toSet();

  // Content that has been added
  QStringList addedFiles = (newFileSet - curFileSet).toList();
  QStringList addedDirectories = (newDirectorySet - curDirectorySet).toList();

  // Content that has been removed
  QStringList deletedFiles = (curFileSet - newFileSet).toList();
  QStringList deletedDirectories = (curDirectorySet - newDirectorySet).toList();

  // Update the current set
  m_workspaceContent[path].files = newFiles;
  m_workspaceContent[path].directories = newDirectories;

  //Handle Files
  if(!addedFiles.isEmpty() || !deletedFiles.isEmpty())
  {
    // File is renamed
    if(addedFiles.count() == 1 && deletedFiles.count() == 1)
    {
      //File Renamed
      static_cast<ROSProjectNode *>(m_project->rootProjectNode())->renameFile(path, deletedFiles.first(), addedFiles.first());
      QString temp = QString::fromLatin1("%1/%2").arg(path, deletedFiles.first());
      m_workspaceFiles.removeAll(QString::fromLatin1("%1/%2").arg(path, deletedFiles.first()));
      m_workspaceFiles.append(QString::fromLatin1("%1/%2").arg(path, addedFiles.first()));
    }
    else
    {
      // New File Added to Dir
      if(!addedFiles.isEmpty())
      {
        for (const QString& file : addedFiles)
        {
          static_cast<ROSProjectNode *>(m_project->rootProjectNode())->addFile(path, file);
          m_workspaceFiles.append(dir.absoluteFilePath(file));
        }
      }

      // File is deleted from Dir
      if(!deletedFiles.isEmpty())
      {
        for (const QString& file : deletedFiles)
        {
          static_cast<ROSProjectNode *>(m_project->rootProjectNode())->removeFile(path, file);
          m_workspaceFiles.removeAll(QString::fromLatin1("%1/%2").arg(path, file));
        }
      }
    }
    ProjectExplorer::ProjectTree::emitSubtreeChanged(m_project->rootProjectNode());
    emit fileListChanged();
  }

  //Handle Directories
  if(!addedDirectories.isEmpty() || !deletedDirectories.isEmpty())
  {
    if(addedDirectories.count() == 1 && deletedDirectories.count() == 1)
    {
      // Directory is renamed
      renameFolder(path, deletedDirectories.first(), addedDirectories.first());
    }
    else
    {
      // New Directory Added to Dir
      for (const QString& directory : addedDirectories)
      {
        watchFolder(path, directory);
      }

      // Directory is deleted from Dir
      for (const QString& directory : deletedDirectories)
      {
        //Directory deleted
        unwatchFolder(path, directory);
      }
    }
  }
}

QStringList ROSWorkspaceWatcher::getWorkspaceFiles()
{
  return m_workspaceFiles;
}

void ROSWorkspaceWatcher::print()
{
  QHashIterator<QString, ROSUtils::FolderContent> item(m_workspaceContent);
  qDebug() << "Workspace Content:";
  while (item.hasNext())
  {
    item.next();
    qDebug() << "Parent:" << item.key();
    qDebug() << "  Files: ";
    for (const QString& str : item.value().files)
      qDebug() << "    " << str;

    qDebug() << "  SubDirectories: ";
    for (const QString& str : item.value().directories)
      qDebug() << "    " << str;
  }

  qDebug() << "File List:";
  for (const QString& str : m_workspaceFiles)
    qDebug() << "  " << str;

}
}
}
