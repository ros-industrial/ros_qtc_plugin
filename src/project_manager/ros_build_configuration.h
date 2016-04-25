#ifndef ROSBUILDCONFIGURATION_H
#define ROSBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/environmentwidget.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <QCheckBox>
#include <QProcess>

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfigurationFactory;
class ROSBuildSettingsWidget;
class ROSBuildInfo;
namespace Ui { class ROSBuildConfiguration; }

class ROSBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class ROSBuildConfigurationFactory;

public:
    explicit ROSBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget() override;
    QList<ProjectExplorer::NamedWidget *> createSubConfigWidgets() override;
    BuildType buildType() const override;
    QVariantMap toMap() const override;

    /**
     * @brief Set the initial build arguments for catkin_make.
     * @param arguments a QString of arguments.
     */
    void setInitialArguments(const QString &arguments);

    /**
     * @brief Get the initial build arguments for catkin_make.
     * @return a QString
     */
    QString initialArguments() const;

    /**
     * @brief Set the ros distribution for the build configuration.
     * @param distribution a QString representing the ros distribution.
     */
    void setROSDistribution(const QString &distribution);

    /**
     * @brief Get the ros distribution of the build configuration.
     * @return a QString.
     */
    QString rosDistribution() const;

    /**
     * @brief Source the workspace to setup the build configuration
     * environment variables.
     */
    void sourceWorkspace();

protected:
    ROSBuildConfiguration(ProjectExplorer::Target *parent, ROSBuildConfiguration *source);
    ROSBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    bool fromMap(const QVariantMap &map) override;

    friend class ROSBuildSettingsWidget;

private:
    QString m_initialArguments; /**< Initial catkin_make arguments. */
    QString m_rosDistribution; /**< ROS Distribution */
    ProjectExplorer::NamedWidget *m_buildEnvironmentWidget; /**< Build configuration environment widget */

};

class ROSBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit ROSBuildConfigurationFactory(QObject *parent = 0);
    ~ROSBuildConfigurationFactory();

    int priority(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo *> availableBuilds(const ProjectExplorer::Target *parent) const override;
    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    QList<ProjectExplorer::BuildInfo *> availableSetups(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent, const ProjectExplorer::BuildInfo *info) const override;
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const override;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;

private:
    /**
     * @brief Check whether target is supported.
     * @param t a ProjectExplorer::Target
     * @return True if valid, otherwise false.
     */
    bool canHandle(const ProjectExplorer::Target *t) const;

    /**
     * @brief Represents all available catkin_make build types.
     */
    enum BuildType { BuildTypeNone = 0,
                     BuildTypeDebug = 1,
                     BuildTypeRelease = 2,
                     BuildTypeRelWithDebInfo = 3,
                     BuildTypeMinSizeRel = 4,
                     BuildTypeLast = 5 };

    /**
     * @brief Creates a ROSBuildInfo
     * @param k a ProjectExplorer::Kit
     * @param projectPath a path to the project.
     * @param type BuildType
     * @return a ROSBuildInfo
     */
    ROSBuildInfo *createBuildInfo(const ProjectExplorer::Kit *k, const QString &projectPath, BuildType type) const;
};

/**
 * @brief The ROS settings widget. This provides a UI for changing ROS
 * build setting.
 */
class ROSBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    ROSBuildSettingsWidget(ROSBuildConfiguration *bc);
    ~ROSBuildSettingsWidget();

private slots:
    /**
     * @brief A slot that is called anytime the source button is clicked.
     *
     * It will source the workspace and setup environment variables.
     */
    void on_source_pushButton_clicked();

    /**
     * @brief A slot that is called anytime the ros distribution is changed.
     * @param arg1 a QString representing the ros distribution.
     */
    void on_ros_distribution_comboBox_currentIndexChanged(const QString &arg1);

private:
    Ui::ROSBuildConfiguration *m_ui; /**< ROS Build configuration UI object. */
    ROSBuildConfiguration *m_buildConfiguration; /**< Pointer to the ROSBuildConfiguration. */
    QMap<QString, QString> m_rosDistributions; /**< Map of available ros distributions. */
};

/**
 * @brief The ROS build environment widget. This provides a UI for changing
 * environment variables unique to the project.
 */
class ROSBuildEnvironmentWidget : public ProjectExplorer::NamedWidget
{
  Q_OBJECT

public:
  ROSBuildEnvironmentWidget(ProjectExplorer::BuildConfiguration *bc);

private slots:
  /**
   * @brief A slot that is called anytime a environment variable is changed
   * and passes the changes to the ROSBuildConfiguration.
   */
  void environmentModelUserChangesChanged();

  /**
   * @brief A slot that is called anytime the clear system environment is
   * checked.
   *
   * It will set the ROSBuildConfiguration environment back to the base
   * environment.
   * @param checked a bool.
   */
  void clearSystemEnvironmentCheckBoxClicked(bool checked);

  /** @brief A slot that is called anytime a environment variable is changed within
   * the ROSBuildConfiguration and updates the change with the Widget.
   */
  void environmentChanged();

protected:
  ProjectExplorer::EnvironmentWidget *m_buildEnvironmentWidget; /**< The environment widget. */
  QCheckBox *m_clearSystemEnvironmentCheckBox; /**< The clear system evironment checkbox object. */
  ProjectExplorer::BuildConfiguration *m_buildConfiguration; /** The build configuration associated to this widget. */
};

/**
 * @brief This class represents all information required for building
 * a ROS project.
 */
class ROSBuildInfo : public ProjectExplorer::BuildInfo
{
public:
    ROSBuildInfo(const ProjectExplorer::IBuildConfigurationFactory *f) :
        ProjectExplorer::BuildInfo(f) { }

    ROSBuildInfo(const Internal::ROSBuildConfiguration *bc) :
        ProjectExplorer::BuildInfo(ProjectExplorer::IBuildConfigurationFactory::find(bc->target()))
    {
        displayName = bc->displayName();
        buildDirectory = bc->buildDirectory();
        kitId = bc->target()->kit()->id();
        environment = bc->environment();
        arguments = bc->initialArguments();
    }

    Utils::Environment environment; /**< ROS build environment. */
    QString arguments; /**< ROS catkin_make arguments. */
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSBUILDCONFIGURATION_H
