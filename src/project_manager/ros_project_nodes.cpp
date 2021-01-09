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
#include "ros_project_constants.h"

#include <utils/fileutils.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/projectexplorer.h>

#include <QFileInfo>

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

ROSProjectNode::ROSProjectNode(const Utils::FilePath &projectFilePath) : ProjectNode(projectFilePath)
{
    setDisplayName(projectFilePath.toFileInfo().completeBaseName());
}

FileNode *ROSProjectNode::findFileNode(FolderNode *folder_node, const Utils::FilePath &filePaths)
{
  FileNode* file_node = folder_node->fileNode(filePaths);
  if (file_node)
      return file_node;

  for (FolderNode *fn : folder_node->folderNodes())
  {
    file_node = findFileNode(fn, filePaths);
    if (file_node)
        return file_node;
  }

  return nullptr;
}

bool ROSProjectNode::showInSimpleTree() const
{
    return true;
}

ROSFolderNode::ROSFolderNode(const Utils::FilePath &folderPath) : FolderNode(folderPath), m_repository(nullptr)
{
    QString path = this->filePath().toString();

    if (Core::IVersionControl *vc = Core::VcsManager::findVersionControlForDirectory(path))
    {
        if (path == Core::VcsManager::findTopLevelForDirectory(path))
        {
            m_repository = vc;
        }
    }
}

QString ROSFolderNode::displayName() const
{
    if (m_repository)
    {
        QString path = this->filePath().toString();
        QString name = Utils::FilePath::fromString(path).fileName();
        return QString::fromLatin1("%1 [%2]").arg(name, m_repository->vcsTopic(path));
    }
    else
    {
        return this->FolderNode::displayName();
    }
}

} // namespace Internal
} // namespace ROSProjectManager
