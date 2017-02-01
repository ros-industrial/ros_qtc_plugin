/**
 * @author Levi Armstrong
 * @date January 1, 2016
 *
 * @copyright Copyright (c) 2016, Southwest Research Institute
 *
 * @license Software License Agreement (Apache License)\n
 * \n
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at\n
 * \n
 * http://www.apache.org/licenses/LICENSE-2.0\n
 * \n
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef ROSRUNSTEPS_H
#define ROSRUNSTEPS_H

#include "ros_run_step.h"
#include "ros_run_configuration.h"

#include <QStringListModel>

namespace ROSProjectManager {
namespace Internal {


namespace Ui { class ROSGenericStep; }

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
    void updateAvailablePackages();

    Ui::ROSGenericStep *m_ui;
    ROSGenericRunStep *m_rosGenericStep;
    QMap<QString, QString> m_availablePackages;
    QStringListModel *m_packageNames;
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
