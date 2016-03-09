#ifndef ROSUTILS_H
#define ROSUTILS_H

#include <QProcess>
#include <QXmlStreamWriter>

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
  static bool gererateQtCreatorWorkspaceFile(QXmlStreamWriter &file, const QStringList &files, const QStringList &includePaths);
  static QStringList getWorkspaceFiles(const Utils::FileName &workspaceDir);
  static QStringList getWorkspaceIncludes(const Utils::FileName &workspaceDir);
  static QMap<QString, QString> getROSPackages(const QStringList &env);
  static QStringList getROSPackageLaunchFiles(const QString &packagePath, bool OnlyNames = true);
  static QStringList getROSPackageExecutables(const QString &packageName, const QStringList &env);

private:
  static bool sourceWorkspaceHelper(QProcess *process, const QString &path);

};

}
}

#endif // ROSUTILS_H
