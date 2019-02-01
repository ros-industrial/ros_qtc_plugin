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
#ifndef ROSPROJECT_H
#define ROSPROJECT_H

#include "ros_project_plugin.h"
#include "ros_utils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>
#include <cpptools/cpprawprojectpart.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QTimer>
#include <QFileSystemWatcher>

namespace CppTools {
    class CppProjectUpdater;
}

namespace ROSProjectManager {
namespace Internal {

class ROSProjectFile;
class ROSBuildConfiguration;

class ROSProject : public ProjectExplorer::Project
{
    Q_OBJECT
    friend class ROSProjectPlugin;

public:
    ROSProject(const Utils::FileName &filename);
    ~ROSProject() override;

    void refresh();

    QString distribution() const;
    ROSUtils::BuildSystem defaultBuildSystem() const;
    ROSBuildConfiguration* rosBuildConfiguration() const;

    ROSUtils::PackageInfoMap getPackageInfo() const;
    ROSUtils::PackageBuildInfoMap getPackageBuildInfo() const;

public slots:
    void buildQueueFinished(bool success);
    void fileSystemChanged();

private slots:
    void updateProjectTree();
    void updateCppCodeModel();

protected:
    Project::RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    void asyncUpdate();
    bool saveProjectFile();
    void asyncUpdateCppCodeModel(bool success);
    void updateEnvironment();

    static void buildProjectTree(const Utils::FileName& projectFilePath,
                                 const QStringList& watchDirectories,
                                 QFutureInterface<ProjectExplorer::ProjectNode*> &fi,
                                 QHash<QString, ROSUtils::FolderContent> &workspaceContent,
                                 QStringList &files,
                                 QStringList &directories);

    static void buildCppCodeModel(const ROSUtils::WorkspaceInfo& workspaceInfo,
                                  const Utils::FileName& projectFilePath,
                                  const QStringList workspaceFiles,
                                  const ProjectExplorer::Kit *k,
                                  QFutureInterface<CppTools::RawProjectParts> &fi,
                                  ROSUtils::PackageInfoMap& wsPackageInfo,
                                  ROSUtils::PackageBuildInfoMap& wsPackageBuildInfo,
                                  Utils::Environment& wsEnvironment);

    ROSUtils::ROSProjectFileContent m_projectFileContent;
    ROSUtils::PackageInfoMap        m_wsPackageInfo;
    ROSUtils::PackageBuildInfoMap   m_wsPackageBuildInfo;
    Utils::Environment              m_wsEnvironment;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater;

    // Watching Directories to keep Project Tree updated
    QTimer m_asyncUpdateTimer;
    QFutureInterface<ProjectExplorer::ProjectNode*> *m_asyncUpdateFutureInterface = nullptr;
    QFileSystemWatcher m_watcher;
    QHash<QString, ROSUtils::FolderContent> m_workspaceContent;
    QStringList m_workspaceFiles;
    QStringList m_workspaceDirectories;
    QFutureWatcher<ProjectExplorer::ProjectNode*> m_futureWatcher;

    // Parse Code Blocks Files and build Code Model
    QFutureInterface<CppTools::RawProjectParts> *m_asyncBuildCodeModelFutureInterface = nullptr;
    QFutureWatcher<CppTools::RawProjectParts> m_futureBuildCodeModelWatcher;

};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECT_H
