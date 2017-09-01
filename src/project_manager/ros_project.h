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


#include "ros_workspace_watcher.h"
#include "ros_project_manager.h"
#include "ros_project_nodes.h"
#include "ros_project_plugin.h"
#include "ros_utils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>

#include <QFuture>
#include <QFutureInterface>

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
    //ROSProject(ROSManager *manager, const QString &filename);
    ROSProject(const Utils::FileName &filename);
    ~ROSProject() override;

    ROSManager *projectManager() const;
    QStringList files(FilesMode fileMode) const;

    void refresh();

    QString distribution() const;
    ROSUtils::BuildSystem defaultBuildSystem() const;
    ROSBuildConfiguration* rosBuildConfiguration() const;

    ROSUtils::PackageInfoMap getPackageInfo() const;
    ROSUtils::PackageBuildInfoMap getPackageBuildInfo() const;

public slots:
    void buildQueueFinished(bool success);

protected:
    Project::RestoreResult fromMap(const QVariantMap &map, QString *errorMessage) override;

private:
    bool saveProjectFile();
    void parseProjectFile();

    void update();
    void refreshCppCodeModel();
    void repositoryChanged(const QString &repository);

    ROSUtils::ROSProjectFileContent m_projectFileContent;
    QFutureInterface<void>         *m_projectFutureInterface = nullptr;
    ROSUtils::PackageInfoMap        m_wsPackageInfo;
    ROSUtils::PackageBuildInfoMap   m_wsPackageBuildInfo;
    Utils::Environment              m_wsEnvironment;

    CppTools::CppProjectUpdater *m_cppCodeModelUpdater;
    ROSWorkspaceWatcher         *m_workspaceWatcher;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECT_H
