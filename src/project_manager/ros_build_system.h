#pragma once

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/projectnodes.h>
#include "ros_build_configuration.h"

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
    virtual bool renameFiles(ProjectExplorer::Node *context, const Utils::FilePairs &filesToRename, Utils::FilePaths *notRenamed) override final;
    virtual bool addDependencies(ProjectExplorer::Node *context, const QStringList &dependencies) override final;
    virtual bool supportsAction(ProjectExplorer::Node *context, ProjectExplorer::ProjectAction action, const ProjectExplorer::Node *node) const override final;
    virtual QString name() const override final;

private:
    const ROSUtils::BuildSystem ros_build_system;
};

} // namespace Internal
} // namespace ROSProjectManager
