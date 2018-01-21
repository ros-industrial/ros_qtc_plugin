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

class ROSProject;

class ROSProjectNode : public ProjectExplorer::ProjectNode
{
public:
    ROSProjectNode(const Utils::FileName &projectFilePath);

    bool showInSimpleTree() const override;

    bool supportsAction(ProjectExplorer::ProjectAction action, const Node *node) const override;

    //These are now handled by the ROSProject manager.
    bool addFiles(const QStringList &filePaths, QStringList *notAdded = 0) override {Q_UNUSED(filePaths); Q_UNUSED(notAdded); return true;}
    bool removeFiles(const QStringList &filePaths, QStringList *notRemoved = 0) override {Q_UNUSED(filePaths); Q_UNUSED(notRemoved); return true;}
    bool deleteFiles(const QStringList &filePaths) override {Q_UNUSED(filePaths); return true;}
    bool renameFile(const QString &filePath, const QString &newFilePath) override {Q_UNUSED(filePath); Q_UNUSED(newFilePath); return true;}

    //These are used by ROSProject QFileSystemWatcher
    bool removeFile(const QString &parentPath, const QString &fileName);
    bool addFile(const QString &parentPath, const QString &fileName);
    bool renameFile(const QString &parentPath, const QString &oldFileName, const QString &newFileName);
    bool removeDirectory(const QString &parentPath, const QString &dirName);
    bool addDirectory(const QString &parentPath, const QString &dirName);
    bool addDirectory(const QString &dirPath);
    bool renameDirectory(const QString &parentPath, const QString &oldDirName, const QString &newDirName); 

private:
    void renameDirectoryHelper(FolderNode *&folder);
    FolderNode *findFolderbyAbsolutePath(const QString &absolutePath);
    FolderNode *createFolderbyAbsolutePath(const QString &absolutePath);
    QString getFolderName(FolderNode *folderNode) const;
    QString getFolderPath(FolderNode *folderNode) const;
};

class ROSFolderNode: public ProjectExplorer::FolderNode
{
public:
    explicit ROSFolderNode(const Utils::FileName &folderPath, const QString  &displayName) : FolderNode(folderPath, ProjectExplorer::NodeType::Folder, displayName), m_repository(nullptr)
    {
        QString path = this->filePath().toString();
        path.chop(1);

        if (Core::IVersionControl *vc = Core::VcsManager::findVersionControlForDirectory(path))
        {
            if (path == Core::VcsManager::findTopLevelForDirectory(path))
            {
                m_repository = vc;
            }
        }
    }
    QString displayName() const override
    {
        if (m_repository)
        {
            QString path = this->filePath().toString();
            path.chop(1);
            QString name = Utils::FileName::fromString(path).fileName();
            return QString::fromLatin1("%1 [%2]").arg(name, m_repository->vcsTopic(path));
        }
        else
        {
            return this->FolderNode::displayName();
        }
    }
private:

    Core::IVersionControl *m_repository;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECTNODE_H
