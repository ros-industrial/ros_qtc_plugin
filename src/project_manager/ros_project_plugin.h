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
     * @brief This will reload the project include directories.
     *
     * If libraries are add to a CMakeList.txt this must me called so
     * the proper include directories get add to the cpp model.
     */
    void reloadProjectIncludeDirectories();

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
