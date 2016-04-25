#ifndef ROSMAKESTEP_H
#define ROSMAKESTEP_H

#include <projectexplorer/abstractprocessstep.h>
#include "ros_build_configuration.h"

QT_BEGIN_NAMESPACE
class QListWidgetItem;
QT_END_NAMESPACE

namespace ROSProjectManager {
namespace Internal {

class ROSMakeStepConfigWidget;
class ROSMakeStepFactory;
namespace Ui { class ROSMakeStep; }

class ROSMakeStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

    friend class ROSMakeStepConfigWidget;
    friend class ROSMakeStepFactory;

public:
    ROSMakeStep(ProjectExplorer::BuildStepList *parent);
    ~ROSMakeStep();

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    QVariantMap toMap() const override;

    /**
     * @brief Check if target is an active build target.
     * @param target a QString name of target to check.
     * @return True if an active build target, otherwise returns false.
     */
    bool buildsTarget(const QString &target) const;

    /**
     * @brief Set a build target active status.
     * @param target a QString name of target.
     * @param on a bool representing the status of the build target.
     */
    void setBuildTarget(const QString &target, bool on);

    /**
     * @brief Get all catkin_make arguments.
     * @param initial_arguments a QString of initial arguments to be prepended.
     * @return a QString of all arguments.
     */
    QString allArguments(QString initial_arguments) const;

    /**
     * @brief Get the make command for the ROS project.
     * @return a QString.
     */
    QString makeCommand() const;

    /**
     * @brief Set the make status to clean.
     * @param clean a bool representing the clean status.
     */
    void setClean(bool clean);

    /**
     * @brief Check if make status is clean.
     * @return a bool.
     */
    bool isClean() const;


    /**
     * @brief Get the ROS build configuration.
     * @return a ROSBuildConfiguration.
     */
    ROSBuildConfiguration *rosBuildConfiguration() const;

protected:
    ROSMakeStep(ProjectExplorer::BuildStepList *parent, ROSMakeStep *bs);
    ROSMakeStep(ProjectExplorer::BuildStepList *parent, Core::Id id);
    bool fromMap(const QVariantMap &map) override;

private:
    /**
     * @brief Constructor routine.
     */
    void ctor();

    /**
     * @brief Get active build configuration.
     * @return a ROSBuildConfiguration
     */
    ROSBuildConfiguration *targetsActiveBuildConfiguration() const;

    QStringList m_buildTargets; /**< A QStringList of active build configurations. */
    QString m_makeArguments; /**< A QString of catkin_make arguments. */
    QString m_makeCommand; /**< A QString of ROS make command catkin_make. */
    bool m_clean; /**< A bool representing the clean status of the make. */
};

class ROSMakeStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    ROSMakeStepConfigWidget(ROSMakeStep *makeStep);
    ~ROSMakeStepConfigWidget();
    QString displayName() const override;
    QString summaryText() const override;

private slots:
    /**
     * @brief A slot that is called anytime a target option are change.
     * @param item a QListWidgetItem that was changed.
     */
    void itemChanged(QListWidgetItem *item);

    /**
     * @brief A slot that is called anytime the make command are edited.
     */
    void makeLineEditTextEdited();

    /**
     * @brief A slot that is called anytime the make arguments are edited.
     */
    void makeArgumentsLineEditTextEdited();

    /**
     * @brief A slot that is called to update the override label.
     */
    void updateMakeOverrrideLabel();

    /**
     * @brief A slot that is called anytime any component of the make
     * configuration widget is chaned to update all necessary information.
     */
    void updateDetails();

private:
    Ui::ROSMakeStep *m_ui; /**< A UI of the ros make step */
    ROSMakeStep *m_makeStep; /**< The ROSMakeStep object */
    QString m_summaryText; /**< The summary text showing the actual command that is executed. */
};

class ROSMakeStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit ROSMakeStepFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *bc) const override;
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(ProjectExplorer::BuildStepList *parent, Core::Id id) const override;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) override;
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const override;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) override;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSMAKESTEP_H
