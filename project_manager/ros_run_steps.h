#ifndef ROSRUNSTEPS_H
#define ROSRUNSTEPS_H

#include "ros_run_step.h"
#include "ros_run_configuration.h"

namespace ROSProjectManager {
namespace Internal {


namespace Ui { class ROSLaunchStep; }

class ROSLaunchStep : public RunStep
{
    Q_OBJECT
    friend class ROSRunStepFactory;

public:
  ROSLaunchStep(RunStepList *parent);
  ~ROSLaunchStep();

  bool init(QList<const RunStep *> &earlierSteps) override;

  void run(QFutureInterface<bool> &fi) override;

  RunStepConfigWidget *createConfigWidget() override;


  QVariantMap toMap() const override;

  ROSRunConfiguration *rosRunConfiguration() const;
  ROSRunConfiguration *targetsActiveRunConfiguration() const;

protected:
  ROSLaunchStep(RunStepList *rsl, Core::Id id);
  ROSLaunchStep(RunStepList *rsl, RunStep *rs);

  bool fromMap(const QVariantMap &map) override;

private:
  QString m_arguments;
  QString m_packageName;
  QString m_target;
};

class ROSLaunchStepConfigWidget : public RunStepConfigWidget
{
  Q_OBJECT
public:
  ROSLaunchStepConfigWidget(ROSLaunchStep *launchStep);
  ~ROSLaunchStepConfigWidget();
  QString summaryText() const override;
  QString displayName() const override;

private:
    Ui::ROSLaunchStep *m_ui;
    ROSLaunchStep *m_rosLaunchStep;

};

class ROSRunStepFactory : public IRunStepFactory
{
    Q_OBJECT

public:
    explicit ROSRunStepFactory(QObject *parent = 0);

    // used to show the list of possible additons to a target, returns a list of types
    QList<Core::Id> availableCreationIds(RunStepList *parent) const override;
    // used to translate the types to names to display to the user
    QString displayNameForId(Core::Id id) const override;

    bool canCreate(RunStepList *parent, Core::Id id) const override;
    RunStep *create(RunStepList *parent, Core::Id id) override;
    // used to recreate the runConfigurations when restoring settings
    bool canRestore(RunStepList *parent, const QVariantMap &map) const override;
    RunStep *restore(RunStepList *parent, const QVariantMap &map) override;
    bool canClone(RunStepList *parent, RunStep *product) const override;
    RunStep *clone(RunStepList *parent, RunStep *product) override;
};

} // Internal
} // ROSProjectManager
#endif // ROSRUNSTEPS_H
