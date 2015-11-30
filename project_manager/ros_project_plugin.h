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
    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized() {}

private slots:
    void editFiles();

};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSPROJECTPLUGIN_H
