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

    virtual bool addFiles(ProjectExplorer::Node *context, const Utils::FilePaths &filePaths, Utils::FilePaths *notAdded = nullptr) override final;
    virtual ProjectExplorer::RemovedFilesFromProject removeFiles(ProjectExplorer::Node *context, const Utils::FilePaths &filePaths, Utils::FilePaths *notRemoved = nullptr) override final;
    virtual bool deleteFiles(ProjectExplorer::Node *context, const Utils::FilePaths &filePaths) override final;
    virtual bool canRenameFile(ProjectExplorer::Node *context, const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override final;
    virtual bool renameFile(ProjectExplorer::Node *context, const Utils::FilePath &oldFilePath, const Utils::FilePath &newFilePath) override final;
    virtual bool addDependencies(ProjectExplorer::Node *context, const QStringList &dependencies) override final;
    virtual bool supportsAction(ProjectExplorer::Node *context, ProjectExplorer::ProjectAction action, const ProjectExplorer::Node *node) const override final;
};

} // namespace Internal
} // namespace ROSProjectManager
