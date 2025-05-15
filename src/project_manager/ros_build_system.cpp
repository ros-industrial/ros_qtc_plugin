#include "ros_build_system.h"
#include "ros_build_configuration.h"

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

ROSBuildSystem::ROSBuildSystem(BuildConfiguration *bc)
    : BuildSystem(bc), ros_build_system(((ROSBuildConfiguration*)(bc))->rosBuildSystem())
{
    connect((bc)->project(), &Project::activeTargetChanged, this, [this]() { triggerParsing(); });
}

void ROSBuildSystem::triggerParsing()
{
    guardParsingRun().markAsSuccess();
}

bool ROSBuildSystem::addFiles(ProjectExplorer::Node *context, const Utils::FilePaths &/*filePaths*/, Utils::FilePaths */*notAdded*/)
{
    // update entire workspace project
    dynamic_cast<ROSProject *>(context->getProject())->refresh();
    return true;
}

ProjectExplorer::RemovedFilesFromProject ROSBuildSystem::removeFiles(ProjectExplorer::Node */*context*/, const Utils::FilePaths &/*filePaths*/, Utils::FilePaths */*notRemoved*/)
{
    return ProjectExplorer::RemovedFilesFromProject::Ok;
}

bool ROSBuildSystem::deleteFiles(ProjectExplorer::Node */*context*/, const Utils::FilePaths &/*filePaths*/)
{
    return true;
}

bool ROSBuildSystem::canRenameFile(ProjectExplorer::Node */*context*/, const Utils::FilePath &/*oldFilePath*/, const Utils::FilePath &/*newFilePath*/)
{
    return true;
}

bool ROSBuildSystem::renameFiles(ProjectExplorer::Node */*context*/, const Utils::FilePairs &/*filesToRename*/, Utils::FilePaths */*notRenamed*/)
{
    return true;
}

bool ROSBuildSystem::addDependencies(ProjectExplorer::Node */*context*/, const QStringList &/*dependencies*/)
{
    return true;
}

bool ROSBuildSystem::supportsAction(ProjectExplorer::Node */*context*/, ProjectExplorer::ProjectAction action, const ProjectExplorer::Node */*node*/) const
{
    static const std::set<ProjectAction> possible_actions = {
        ProjectAction::AddNewFile,
        ProjectAction::AddExistingFile,
        ProjectAction::AddExistingDirectory,
        ProjectAction::RemoveFile,
        ProjectAction::EraseFile,
        ProjectAction::Rename,
    };

    return possible_actions.count(action);
}

} // namespace Internal
} // namespace ROSProjectManager
