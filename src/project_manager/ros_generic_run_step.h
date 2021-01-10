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
#ifndef ROS_GENERIC_RUN_STEP_H
#define ROS_GENERIC_RUN_STEP_H

#include "ros_run_step.h"
#include "ros_run_configuration.h"

namespace ROSProjectManager {
namespace Internal {


namespace Ui { class ROSGenericStep; }

class ROSGenericRunStep : public RunStep
{
    Q_OBJECT
    friend class ROSGenericRunStepConfigWidget;

public:
    ROSGenericRunStep(RunStepList *rsl, Utils::Id id);
    ~ROSGenericRunStep();

    bool init(QList<const RunStep *> &earlierSteps) override;

    void run() override;

    RunStepConfigWidget *createConfigWidget() override;

    QVariantMap toMap() const override;

    ROSRunConfiguration *rosRunConfiguration() const;
    ROSRunConfiguration *targetsActiveRunConfiguration() const;

    virtual QString getCommand() const;
    virtual QString getPackage() const;
    virtual QString getPackagePath() const;
    virtual QString getTarget() const;
    virtual QString getTargetPath() const;
    virtual QString getArguments() const;
    virtual bool getDebugContinueOnAttach() const;

    virtual void setCommand(const QString &command);
    virtual void setPackage(const QString &package);
    virtual void setPackagePath(const QString &package);
    virtual void setTarget(const QString &target);
    virtual void setTargetPath(const QString &target);
    virtual void setArguments(const QString &arguments);
    virtual void setDebugContinueOnAttach(const bool &contOnAttach);

protected:
    bool fromMap(const QVariantMap &map) override;

    virtual QMap<QString, QString> getAvailableTargets() { return QMap<QString, QString>(); }

private:
    QString m_command;
    QString m_package;
    QString m_package_path;
    QString m_target;
    QString m_targetPath;
    QString m_arguments;

    bool m_debugContinueOnAttach;
};

class ROSGenericRunStepConfigWidget : public RunStepConfigWidget
{
    Q_OBJECT

public:
    ROSGenericRunStepConfigWidget(ROSGenericRunStep *genericStep, bool packages_show, bool targets_show, bool debug_show);
    ~ROSGenericRunStepConfigWidget();
    QString summaryText() const override;
    QString displayName() const override;

private slots:
    void debugCheckBox_toggled(const bool &arg1);

    void packageComboBox_currentIndexChanged(const QString &arg1);

    void targetComboBox_currentIndexChanged(const QString &arg1);

    void argumentsLineEdit_textChanged(const QString &arg1);

private:
      void updateAvailableTargets();
      void updateAvailablePackages();

      Ui::ROSGenericStep *m_ui;
      ROSGenericRunStep *m_rosGenericStep;
      QMap<QString, QString> m_availablePackages;
      QMap<QString, QString> m_availableTargets;
      QStringListModel *m_packageNames;
      QStringListModel *m_targetNames;
};

} // Internal
} // ROSProjectManager
#endif // ROS_GENERIC_RUN_STEP_H
