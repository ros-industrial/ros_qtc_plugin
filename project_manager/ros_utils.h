#ifndef ROSUTILS_H
#define ROSUTILS_H

#include <QProcess>
namespace Utils {
class FileName;
} // namespace Utils

namespace ROSProjectManager {
namespace Internal {

class ROSUtils
{
public:
  ROSUtils();
  static bool generateCodeBlocksProjectFile(QProcess *process, const Utils::FileName &sourceDir, const Utils::FileName &buildDir);
  static bool sourceROS(QProcess *process, const QString &rosDistribution);
  static bool sourceWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution);
  static bool isWorkspaceInitialized(const Utils::FileName &workspaceDir);
  static bool hasDevelDirectory(const Utils::FileName &workspaceDir);
  static bool hasBuildDirectory(const Utils::FileName &workspaceDir);
  static bool initializeWorkspace(QProcess *process, const Utils::FileName &workspaceDir, const QString &rosDistribution);
  static bool buildWorkspace(QProcess *process, const Utils::FileName &workspaceDir);
  static QStringList installedDistributions();

private:
  static bool sourceWorkspaceHelper(QProcess *process, const QString &path);

};

}
}

#endif // ROSUTILS_H
