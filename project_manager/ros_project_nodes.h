#ifndef ROSPROJECTNODE_H
#define ROSPROJECTNODE_H

#include <projectexplorer/projectnodes.h>
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

    QList<ProjectExplorer::ProjectAction> supportedActions(Node *node) const override;

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
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECTNODE_H
