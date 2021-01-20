#pragma once

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectnodes.h>

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfiguration;

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

class ROSBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit ROSBuildSystem(ROSBuildConfiguration *bc);

    void triggerParsing() final;

    virtual bool addFiles(ProjectExplorer::Node *context, const QStringList &filePaths, QStringList *notAdded = nullptr) final;
    virtual ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context, const QStringList &filePaths,
                                                                 QStringList *notRemoved = nullptr) final;
    virtual bool deleteFiles(ProjectExplorer::Node *context, const QStringList &filePaths) final;
    virtual bool canRenameFile(ProjectExplorer::Node *context, const QString &filePath, const QString &newFilePath) final;
    virtual bool renameFile(ProjectExplorer::Node *context, const QString &filePath, const QString &newFilePath) final;
    virtual bool addDependencies(ProjectExplorer::Node *context, const QStringList &dependencies) final;
    virtual bool supportsAction(ProjectExplorer::Node *context, ProjectExplorer::ProjectAction action, const ProjectExplorer::Node *node) const final;
};

} // namespace Internal
} // namespace ROSProjectManager
