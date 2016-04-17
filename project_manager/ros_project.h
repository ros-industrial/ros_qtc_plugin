#ifndef ROSPROJECT_H
#define ROSPROJECT_H

#include "ros_project_manager.h"
#include "ros_project_nodes.h"
#include "ros_utils.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/buildconfiguration.h>
#include <coreplugin/idocument.h>

#include <QFuture>
#include <QFileSystemWatcher>

namespace ROSProjectManager {
namespace Internal {

class ROSProjectFile;

class ROSProject : public ProjectExplorer::Project
{
    Q_OBJECT

public:
    ROSProject(ROSManager *manager, const QString &filename);
    ~ROSProject() override;

    QString displayName() const override;
    ROSManager *projectManager() const override;
    QStringList files(FilesMode fileMode) const override;

    QStringList buildTargets() const;

    bool addIncludes(const QStringList &includePaths);
    bool setIncludes(const QStringList &includePaths);

    void refresh();

    QStringList projectIncludePaths() const;
    QStringList workspaceFiles() const;

    Utils::FileName buildDirectory() const;
    Utils::FileName sourceDirectory() const;

public slots:
    void onDirectoryChanged(const QString &path);

protected:
    Project::RestoreResult fromMap(const QVariantMap &map, QString *errorMessage);

private:
    void addDirectory(const QString &parentPath, const QString &dirName);
    void removeDirectory(const QString &parentPath, const QString &dirName);
    void renameDirectory(const QString &parentPath, const QString &oldDirName, const QString &newDirName);
    bool saveProjectFile();
    void parseProjectFile();
    void refreshCppCodeModel();
    void print();

    QString m_projectName;
    QHash<QString, ROSUtils::FolderContent> m_workspaceContent;
    QStringList m_watchDirectories;
    QStringList m_workspaceFiles;
    QStringList m_projectIncludePaths;
    QFileSystemWatcher m_watcher;
    QFuture<void> m_codeModelFuture;
};

class ROSProjectFile : public Core::IDocument
{
    Q_OBJECT

public:
    ROSProjectFile(ROSProject *parent, QString fileName);

    bool save(QString *errorString, const QString &fileName, bool autoSave) override;

    bool isModified() const override;
    bool isSaveAsAllowed() const override;

    ReloadBehavior reloadBehavior(ChangeTrigger state, ChangeType type) const override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    ROSProject *m_project;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECT_H
