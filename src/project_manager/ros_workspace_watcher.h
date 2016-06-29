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
#ifndef ROS_WORKSPACE_WATCHER_H
#define ROS_WORKSPACE_WATCHER_H

#include "ros_utils.h"
#include <projectexplorer/projectnodes.h>
#include <QFileSystemWatcher>


namespace ROSProjectManager {
namespace Internal {

class ROSProject;

class ROSWorkspaceWatcher : public QObject
{
  Q_OBJECT
public:
  ROSWorkspaceWatcher(ROSProject *parent);

  void watchFolder(const QString &parentPath, const QString &folderName);
  void unwatchFolder(const QString &parentPath, const QString &folderName);

  QStringList getWorkspaceFiles();
  void print();

public slots:
  void onFolderChanged(const QString &path);

signals:
  void fileListChanged();

private:
  void renameFolder(const QString &parentPath, const QString &oldFolderName, const QString &newFolderName);

  ROSProject *m_project;
  QFileSystemWatcher m_watcher;
  QHash<QString, ROSUtils::FolderContent> m_workspaceContent;
  QStringList m_workspaceFiles;
};

}
}

#endif // ROS_WORKSPACE_WATCHER_H
