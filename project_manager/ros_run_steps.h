#ifndef ROSRUNSTEPS_H
#define ROSRUNSTEPS_H

#include "ros_run_step.h"
#include "ros_run_configuration.h"

namespace ROSProjectManager {
namespace Internal {


namespace Ui { class ROSGenericStep; }

//class ROSLaunchStep : public RunStep
//{
//    Q_OBJECT
//    friend class ROSRunStepFactory;

//public:
//  ROSLaunchStep(RunStepList *parent);
//  ~ROSLaunchStep();

//  bool init(QList<const RunStep *> &earlierSteps) override;

//  void run(QFutureInterface<bool> &fi) override;

//  RunStepConfigWidget *createConfigWidget() override;


//  QVariantMap toMap() const override;

//  ROSRunConfiguration *rosRunConfiguration() const;
//  ROSRunConfiguration *targetsActiveRunConfiguration() const;

//  QString packageName() const;
//  QString launchFileName() const;
//  QString launchFileArguments() const;

//  void setPackageName(const QString &packageName);
//  void setLaunchFileName(const QString &launchFileName);
//  void setLaunchFileArguments(const QString &launchFileArguments);

//protected:
//  ROSLaunchStep(RunStepList *rsl, Core::Id id);
//  ROSLaunchStep(RunStepList *rsl, RunStep *rs);

//  bool fromMap(const QVariantMap &map) override;

//private:
//  QString m_arguments;
//  QString m_packageName;
//  QString m_target;
//};

//class ROSLaunchStepConfigWidget : public RunStepConfigWidget
//{
//  Q_OBJECT
//public:
//  ROSLaunchStepConfigWidget(ROSLaunchStep *launchStep);
//  ~ROSLaunchStepConfigWidget();
//  QString summaryText() const override;
//  QString displayName() const override;

//private slots:
//  void packageNameComboBox_currentIndexChanged(const QString &arg1);

//  void launchFileComboBox_currentIndexChanged(const QString &arg1);

//  void argumentsLineEdit_textChanged(const QString &arg1);

//private:
//    Ui::ROSLaunchStep *m_ui;
//    ROSLaunchStep *m_rosLaunchStep;
//};


class ROSGenericRunStep : public RunStep
{
    Q_OBJECT
    friend class ROSRunStepFactory;

public:
  ROSGenericRunStep(RunStepList *rsl, Core::Id id);
  ~ROSGenericRunStep();

  bool init(QList<const RunStep *> &earlierSteps) override;

  void run(QFutureInterface<bool> &fi) override;

  RunStepConfigWidget *createConfigWidget() override;

  QVariantMap toMap() const override;

  ROSRunConfiguration *rosRunConfiguration() const;
  ROSRunConfiguration *targetsActiveRunConfiguration() const;

  QString getCommand() const;
  QString getPackage() const;
  QString getTarget() const;
  QString getArguments() const;

  void setPackage(const QString &package);
  void setTarget(const QString &target);
  void setArguments(const QString &arguments);

protected:
  ROSGenericRunStep(RunStepList *rsl, RunStep *rs);

  bool fromMap(const QVariantMap &map) override;

private:
  QString m_command;
  QString m_package;
  QString m_target;
  QString m_arguments;
};

class ROSGenericRunStepConfigWidget : public RunStepConfigWidget
{
  Q_OBJECT
public:
  ROSGenericRunStepConfigWidget(ROSGenericRunStep *genericStep);
  ~ROSGenericRunStepConfigWidget();
  QString summaryText() const override;
  QString displayName() const override;

private slots:
  void packageComboBox_currentIndexChanged(const QString &arg1);

  void targetComboBox_currentIndexChanged(const QString &arg1);

  void argumentsLineEdit_textChanged(const QString &arg1);

private:
    QStringList getAvailableTargets();

    Ui::ROSGenericStep *m_ui;
    ROSGenericRunStep *m_rosGenericStep;
    QMap<QString, QString> m_availablePackages;
};

class ROSRunStep : public ROSGenericRunStep
{
  Q_OBJECT
  friend class ROSRunStepFactory;

public:
  ROSRunStep(RunStepList *rsl);

private:
  ROSRunStep(RunStepList *rsl, Core::Id id);
  ROSRunStep(RunStepList *rsl, RunStep *rs);
};

class ROSLaunchStep : public ROSGenericRunStep
{
  Q_OBJECT
  friend class ROSRunStepFactory;

public:
  ROSLaunchStep(RunStepList *rsl);

private:
  ROSLaunchStep(RunStepList *rsl, Core::Id id);
  ROSLaunchStep(RunStepList *rsl, RunStep *rs);
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
