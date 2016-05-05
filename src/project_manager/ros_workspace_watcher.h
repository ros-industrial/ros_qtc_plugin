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
