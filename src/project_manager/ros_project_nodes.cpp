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
          folder->removeFileNodes(QList<FileNode *>() << fn);
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
  FileType fileType = ResourceType;
  if (Constants::SOURCE_FILE_EXTENSIONS.contains(fileInfo.suffix()))
    fileType = HeaderType;
  else if(Constants::HEADER_FILE_EXTENSIONS.contains(fileInfo.suffix()))
    fileType = SourceType;

  FileNode *fileNode = new FileNode(Utils::FileName::fromString(fileInfo.absoluteFilePath()),
                                    fileType, /*generated = */ false);

  folder->addFileNodes(QList<FileNode *>() << fileNode);
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
  foreach (FolderNode *fn, folder->subFolderNodes())
  {
      if (getFolderName(fn) == dirName)
      {
          folder->removeFolderNodes(QList<FolderNode *>() << fn);
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
  foreach(FolderNode *fn, folder->subFolderNodes())
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

    foreach (FolderNode *fn, parent->subFolderNodes()) {
        if (fn->filePath().toString() == (absolutePath + QLatin1Char('/')))
            return fn;
    }

    return 0;
  }
  else
  {
    return projectNode()->asFolderNode();
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

  parent->addFolderNodes(QList<FolderNode*>() << folderNode);

  return folderNode;
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
