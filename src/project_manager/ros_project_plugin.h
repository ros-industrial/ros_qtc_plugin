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
#ifndef ROSPROJECTPLUGIN_H
#define ROSPROJECTPLUGIN_H
#include <extensionsystem/iplugin.h>

#include <QObject>
#include <QAction>

namespace ProjectExplorer {
class Project;
class Node;
}

namespace ROSProjectManager {
namespace Internal {

class ROSProjectPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "ROSProjectManager.json")

public:
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override {}

private slots:

    /**
     * @brief This will remove the selected FolderNode in the project tree.
     */
    void removeProjectDirectory();

private:

    /**
     * @brief This creates a built-in ROS Cpp code style.
     */
    void createCppCodeStyle();

};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECTPLUGIN_H
