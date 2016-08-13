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
#ifndef ROSPROJECTMANAGER_H
#define ROSPROJECTMANAGER_H

#include <projectexplorer/iprojectmanager.h>
#include <projectexplorer/processparameters.h>
#include <qtermwidget5/qtermwidget.h>
#include "ros_terminal_pane.h"

namespace ROSProjectManager {
namespace Internal {

class ROSProject;

class ROSManager : public ProjectExplorer::IProjectManager
{
    Q_OBJECT

public:
    ROSManager();
    ~ROSManager();

    virtual QString mimeType() const;
    virtual ProjectExplorer::Project *openProject(const QString &fileName, QString *errorString);

    void registerProject(ROSProject *project);
    void unregisterProject(ROSProject *project);

    static ROSManager *instance();

    QTermWidget &startTerminal(int startnow = 1, const QString name = QString());

private:
    QList<ROSProject *> m_projects;
    ROSTerminalPane *m_terminalPane;

};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECTMANAGER_H
