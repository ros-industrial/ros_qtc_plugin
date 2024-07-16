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
#include "ros_build_system.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>
#include <projectexplorer/rawprojectpart.h>
#include <projectexplorer/projectupdater.h>

#include <QFuture>
#include <QFutureWatcher>
#include <QFutureInterface>
#include <QTimer>
#include <QFileSystemWatcher>

namespace ROSProjectManager {
namespace Internal {

class ROSProjectFile;
class ROSBuildConfiguration;

class ROSProject : public ProjectExplorer::Project
{
    Q_OBJECT
    friend class ROSProjectPlugin;

public:
    ROSProject(const Utils::FilePath &filename);
    ~ROSProject() override;

    void refresh();

    Utils::FilePath distribution() const;
    ROSUtils::BuildSystem defaultBuildSystem() const;
    ROSBuildConfiguration* rosBuildConfiguration() const;

    ROSUtils::PackageInfoMap getPackageInfo() const;
    ROSUtils::PackageBuildInfoMap getPackageBuildInfo() const;

public slots:
    void buildQueueFinished(bool success);
    void fileSystemChanged(const QString &path);

private slots:
    void updateProjectTree();
    void updateCppCodeModel();

protected:
    Project::RestoreResult fromMap(const Utils::Store &map, QString *errorMessage) override;

private:
    void asyncUpdate();
    bool saveProjectFile();
    void asyncUpdateCppCodeModel(bool success);
    void updateEnvironment();

    ROSUtils::ROSProjectFileContent m_projectFileContent;
    ROSUtils::PackageInfoMap        m_wsPackageInfo;
    ROSUtils::PackageBuildInfoMap   m_wsPackageBuildInfo;

    ProjectExplorer::ProjectUpdater *m_cppCodeModelUpdater;

    // Watching Directories to keep Project Tree updated
    QTimer m_asyncUpdateTimer;
    QFileSystemWatcher m_watcher;
    QHash<QString, ROSUtils::FolderContent> m_workspaceContent;
    QStringList m_workspaceFiles;
    QStringList m_workspaceDirectories;
    bool m_project_loaded;


    struct FutureWatcherResults
    {
      ProjectExplorer::ProjectNode* node;
      QHash<QString, ROSUtils::FolderContent> workspaceContent;
      QStringList files;
      QStringList directories;
    };

    struct CppToolsFutureResults
    {
      ProjectExplorer::RawProjectParts parts;
      ROSUtils::PackageInfoMap wsPackageInfo;
      ROSUtils::PackageBuildInfoMap wsPackageBuildInfo;
    };

    QFutureInterface<FutureWatcherResults> *m_asyncUpdateFutureInterface;
    QFutureWatcher<FutureWatcherResults> m_futureWatcher;
    // Parse Code Blocks Files and build Code Model
    QFutureInterface<CppToolsFutureResults> *m_asyncBuildCodeModelFutureInterface;
    QFutureWatcher<CppToolsFutureResults> m_futureBuildCodeModelWatcher;

    static void buildProjectTree(const Utils::FilePath projectFilePath,
                                 const Utils::FilePath& sourcePath,
                                 QFutureInterface<FutureWatcherResults> &fi);

    static void buildCppCodeModel(const ROSUtils::WorkspaceInfo workspaceInfo,
                                  const Utils::FilePath projectFilePath,
                                  const QStringList workspaceFiles,
                                  const ProjectExplorer::Kit *k,
                                  const Utils::Environment &env,
                                  const ROSUtils::PackageInfoMap wsPackageInfo,
                                  const ROSUtils::PackageBuildInfoMap  wsPackageBuildInfo,
                                  QFutureInterface<CppToolsFutureResults> &fi);

};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECT_H
