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

  /**
   * @brief installedDistributions - Gets a list of installed ROS Distributions
   * @return QStringList of installed ROS Distribution Names
   */
  static QStringList installedDistributions();

  /**
   * @brief gererateQtCreatorWorkspaceFile - Generates/Updates the Qt ROS Project File
   * @param file - The Qt ROS Project Files
   * @param files - List of Workspace Files
   * @param includePaths - List of required include directories
   * @return A bool whether the Qt ROS Project file was succesfully updated
   */
  static bool gererateQtCreatorWorkspaceFile(QXmlStreamWriter &file, const QStringList &files, const QStringList &includePaths);

  /**
   * @brief getWorkspaceFiles - Gets all fo the files in a ROS Workspace
   * @param workspaceDir - Path to the ROS Workspace
   * @return QStringList of file paths
   */
  static QStringList getWorkspaceFiles(const Utils::FileName &workspaceDir);

  /**
   * @brief getWorkspaceIncludes - Gets all of the include directories
   * @param workspaceDir - Path of the ROS Workspace
   * @return QStringList of include directories
   */
  static QStringList getWorkspaceIncludes(const Utils::FileName &workspaceDir);

  /**
   * @brief getROSPackages - Executes the bash command "rospack list" and returns
   * a map of QMap(Package Name, Path to package)
   * @param env - Is the environment to use for getting the list of available packages.
   * @return QMap(Package Name, Path to package)
   */
  static QMap<QString, QString> getROSPackages(const QStringList &env);

  /**
   * @brief getROSPackageLaunchFiles - Gets all launch file associated to a package
   * @param packagePath - ROS Package Name
   * @param OnlyNames - Flag to determine if you only need the file name versus the path
   * @return QStringList of launch files
   */
  static QStringList getROSPackageLaunchFiles(const QString &packagePath, bool OnlyNames = true);

  /**
   * @brief getROSPackageExecutables - Gets all of the executables associated to a package
   * @param packageName - ROS Package Name
   * @param env - ROS Workspace Environment
   * @return QStringList of executables
   */
  static QStringList getROSPackageExecutables(const QString &packageName, const QStringList &env);

private:
  static bool sourceWorkspaceHelper(QProcess *process, const QString &path);

};

}
}

#endif // ROSUTILS_H
