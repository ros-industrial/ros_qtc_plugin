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
#ifndef ROSPROJECTNODE_H
#define ROSPROJECTNODE_H

#include <projectexplorer/projectnodes.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/iversioncontrol.h>

#include "ros_utils.h"

#include <QStringList>
#include <QHash>
#include <QSet>

namespace Core { class IDocument; }

namespace ROSProjectManager {
namespace Internal {

class ROSProjectNode : public ProjectExplorer::ProjectNode
{
public:
    explicit ROSProjectNode(const Utils::FilePath &projectFilePath);

    bool showInSimpleTree() const override;

    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;

    bool addFiles(const QStringList &filePaths, QStringList *notAdded = nullptr) override;

    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = nullptr) override;

    bool deleteFiles(const QStringList &filePaths) override;

    bool renameFile(const QString &filePath, const QString &newFilePath) override;

private:
    static ProjectExplorer::FileNode *findFileNode(FolderNode *folder_node, const Utils::FilePath &filePaths);
};
typedef std::unique_ptr<ROSProjectNode> ROSProjectNodeUPtr;

class ROSFolderNode: public ProjectExplorer::FolderNode
{
public:
    explicit ROSFolderNode(const Utils::FilePath &folderPath);

    QString displayName() const override;

private:
    Core::IVersionControl *m_repository;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECTNODE_H
