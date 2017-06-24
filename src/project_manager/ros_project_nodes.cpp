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
#include "ros_project_nodes.h"
#include "ros_project.h"
#include "ros_project_constants.h"

#include <utils/fileutils.h>
#include <coreplugin/idocument.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/iversioncontrol.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

ROSProjectNode::ROSProjectNode(const Utils::FileName &projectFilePath)
    : ProjectNode(projectFilePath)
{
    setDisplayName(projectFilePath.toFileInfo().completeBaseName());
}

bool ROSProjectNode::removeFile(const QString &parentPath, const QString &fileName)
{
  FolderNode *folder = findFolderbyAbsolutePath(parentPath);
  foreach (FileNode *fn, folder->fileNodes())
  {
      if (fn->filePath().fileName() == fileName)
      {
#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
          folder->removeFileNodes(QList<FileNode *>() << fn);
#else
          folder->removeNode(fn);
#endif
          return true;
      }
  }
  return false;
}

bool ROSProjectNode::addFile(const QString &parentPath, const QString &fileName)
{
  FolderNode *folder = findFolderbyAbsolutePath(parentPath);
  if(!folder)
    folder = createFolderbyAbsolutePath(parentPath);

  QFileInfo fileInfo(QDir(parentPath), fileName);

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
  FileType fileType = ResourceType;

  if (Constants::SOURCE_FILE_EXTENSIONS.contains(fileInfo.suffix()))
    fileType = HeaderType;
  else if(Constants::HEADER_FILE_EXTENSIONS.contains(fileInfo.suffix()))
    fileType = SourceType;
#else
  FileType fileType = FileType::Resource;

  if (Constants::SOURCE_FILE_EXTENSIONS.contains(fileInfo.suffix()))
    fileType = FileType::Header;
  else if(Constants::HEADER_FILE_EXTENSIONS.contains(fileInfo.suffix()))
    fileType = FileType::Source;
#endif

  FileNode *fileNode = new FileNode(Utils::FileName::fromString(fileInfo.absoluteFilePath()),
                                    fileType, /*generated = */ false);

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
  folder->addFileNodes(QList<FileNode *>() << fileNode);
#else
  folder->addNode(fileNode);
#endif
  return true;
}

bool ROSProjectNode::renameFile(const QString &parentPath, const QString &oldFileName, const QString &newFileName)
{
  FolderNode *folder = findFolderbyAbsolutePath(parentPath);
  foreach (FileNode *fn, folder->fileNodes())
  {
      if (fn->filePath().fileName() == oldFileName)
      {
          QFileInfo fileInfo(QDir(parentPath), newFileName);
          fn->setAbsoluteFilePathAndLine(Utils::FileName::fromString(fileInfo.absoluteFilePath()),-1);
          return true;
      }
  }
  return false;
}

bool ROSProjectNode::removeDirectory(const QString &parentPath, const QString &dirName)
{
  FolderNode *folder = findFolderbyAbsolutePath(parentPath);

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
  foreach (FolderNode *fn, folder->subFolderNodes())
#else
  foreach (FolderNode *fn, folder->folderNodes())
#endif
  {
      if (getFolderName(fn) == dirName)
      {
#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
        folder->removeFolderNodes(QList<FolderNode *>() << fn);
#else
        folder->removeNode(fn);
#endif
        return true;
      }
  }
  return false;
}

bool ROSProjectNode::addDirectory(const QString &parentPath, const QString &dirName)
{
  return addDirectory(QString::fromLatin1("%1/%2").arg(parentPath, dirName));
}

bool ROSProjectNode::addDirectory(const QString &dirPath)
{
  FolderNode *folder = findFolderbyAbsolutePath(dirPath);
  if (!folder)
    createFolderbyAbsolutePath(dirPath);

  return true;
}

bool ROSProjectNode::renameDirectory(const QString &parentPath, const QString &oldDirName, const QString &newDirName)
{
  QString newFilePath = QString::fromLatin1("%1/%2").arg(parentPath, newDirName);
  QString oldFilePath = QString::fromLatin1("%1/%2").arg(parentPath, oldDirName);
  FolderNode *folder = findFolderbyAbsolutePath(oldFilePath);
  Utils::FileName folderPath =  Utils::FileName::fromString(newFilePath + QLatin1Char('/'));
  folder->setAbsoluteFilePathAndLine(folderPath, -1);
  folder->setDisplayName(newDirName);
  updateVersionControlInfoHelper(folder);
  renameDirectoryHelper(folder);

  return true;
}

void ROSProjectNode::renameDirectoryHelper(FolderNode * &folder)
{
  // Update Files
  foreach(FileNode *fn, folder->fileNodes())
  {
    QString newFilePath = folder->filePath().toString() + fn->displayName();
    fn->setAbsoluteFilePathAndLine(Utils::FileName::fromString(newFilePath), -1);
  }

  // Update subFolders
#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
  foreach(FolderNode *fn, folder->subFolderNodes())
#else
  foreach(FolderNode *fn, folder->folderNodes())
#endif
  {
    QString newFilePath = folder->filePath().toString() + getFolderName(fn) + QLatin1Char('/');
    fn->setAbsoluteFilePathAndLine(Utils::FileName::fromString(newFilePath), -1);
    renameDirectoryHelper(fn);
  }
}

FolderNode *ROSProjectNode::findFolderbyAbsolutePath(const QString &absolutePath) const
{
  if (filePath().parentDir().toString() != absolutePath)
  {
    Utils::FileName folder = Utils::FileName::fromString(absolutePath);
    FolderNode *parent = findFolderbyAbsolutePath(folder.parentDir().toString());

    if (!parent)
        return 0;

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
    foreach (FolderNode *fn, parent->subFolderNodes())
#else
    foreach (FolderNode *fn, parent->folderNodes())
#endif
    {
        if (fn->filePath().toString() == (absolutePath + QLatin1Char('/')))
            return fn;
    }

    return 0;
  }
  else
  {
#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
    return projectNode()->asFolderNode();
#else
    return projectNode(Utils::FileName::fromString(absolutePath))->asFolderNode();
#endif
  }
}

FolderNode *ROSProjectNode::createFolderbyAbsolutePath(const QString &absolutePath)
{
  Utils::FileName folder = Utils::FileName::fromString(absolutePath);
  FolderNode *folderNode = new FolderNode(Utils::FileName::fromString(absolutePath + QLatin1Char('/')));
  updateVersionControlInfoHelper(folderNode);

  FolderNode *parent = findFolderbyAbsolutePath(folder.parentDir().toString());

  if (!parent)
      parent = createFolderbyAbsolutePath(folder.parentDir().toString());

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
  parent->addFolderNodes(QList<FolderNode*>() << folderNode);
#else
  parent->addNode(folderNode);
#endif

  return folderNode;
}

bool ROSProjectNode::showInSimpleTree() const
{
    return true;
}

#if QT_CREATOR_VER < QT_CREATOR_VER_CHECK(4,3,0)
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
#else
bool ROSProjectNode::supportsAction(ProjectExplorer::ProjectAction action, Node *node) const
{
    switch (node->nodeType())
    {
        case NodeType::File:
            return action == ProjectAction::Rename
                || action == ProjectAction::RemoveFile;
        case NodeType::Folder:
        case NodeType::Project:
            return action == ProjectAction::AddNewFile
                || action == ProjectAction::RemoveFile
                || action == ProjectAction::AddExistingFile;
        default:
            return ProjectNode::supportsAction(action, node);
    }
}

#endif

bool ROSProjectNode::hasVersionControl(const QString &absolutePath, QString &vcsTopic) const
{
  if (Core::IVersionControl *vc = Core::VcsManager::findVersionControlForDirectory(absolutePath))
  {
    if (absolutePath == Core::VcsManager::findTopLevelForDirectory(absolutePath))
    {
      vcsTopic = vc->vcsTopic(absolutePath);
      return true;
    }
  }
  return false;
}

void ROSProjectNode::updateVersionControlInfo(const QString &absolutePath) const
{
  updateVersionControlInfoHelper(findFolderbyAbsolutePath(absolutePath));
}

void ROSProjectNode::updateVersionControlInfoHelper(FolderNode *folderNode) const
{
  QString branch;
  if (hasVersionControl(getFolderPath(folderNode), branch))
      folderNode->setDisplayName(QString::fromLatin1("%1 [%2]").arg(getFolderName(folderNode), branch));
  else
      folderNode->setDisplayName(getFolderName(folderNode));
}

QString ROSProjectNode::getFolderName(FolderNode *folderNode) const
{
  QString path = getFolderPath(folderNode);
  return Utils::FileName::fromString(path).fileName();
}

QString ROSProjectNode::getFolderPath(FolderNode *folderNode) const
{
  QString path = folderNode->filePath().toString();
  path.chop(1);
  return path;
}

} // namespace Internal
} // namespace ROSProjectManager
