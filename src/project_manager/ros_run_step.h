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
#ifndef RUNSTEP_H
#define RUNSTEP_H

#include <projectexplorer/projectconfiguration.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/task.h>

#include <QFutureInterface>
#include <QWidget>

namespace ROSProjectManager {
namespace Internal {

class RunStepConfigWidget;
class RunStepList;

// Documentation inside.
class RunStep : public ProjectExplorer::ProjectConfiguration
{
    Q_OBJECT

protected:
    RunStep(RunStepList *rsl, Core::Id id);
    RunStep(RunStepList *rsl, RunStep *rs);

public:
    virtual bool init(QList<const RunStep *> &earlierSteps) = 0;

    virtual void run() = 0;

    virtual RunStepConfigWidget *createConfigWidget() = 0;

    virtual bool immutable() const;
    virtual bool runInGuiThread() const;
    virtual void cancel();

    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool enabled() const;
    void setEnabled(bool b);

    ProjectExplorer::RunConfiguration *runConfiguration() const;
    ProjectExplorer::ProjectConfiguration *projectConfiguration() const;
    ProjectExplorer::Target *target() const;
    ProjectExplorer::Project *project() const;

signals:
    void finished();
    void enabledChanged();
private:
    bool m_enabled;
};

class IRunStepFactory : public QObject
{
    Q_OBJECT

public:
    explicit IRunStepFactory(QObject *parent = 0);

    // used to show the list of possible additons to a target, returns a list of types
    virtual QList<Core::Id> availableCreationIds(RunStepList *parent) const = 0;
    // used to translate the types to names to display to the user
    virtual QString displayNameForId(Core::Id id) const = 0;

    virtual bool canCreate(RunStepList *parent, Core::Id id) const = 0;
    virtual RunStep *create(RunStepList *parent, Core::Id id) = 0;
    // used to recreate the runConfigurations when restoring settings
    virtual bool canRestore(RunStepList *parent, const QVariantMap &map) const = 0;
    virtual RunStep *restore(RunStepList *parent, const QVariantMap &map) = 0;
    virtual bool canClone(RunStepList *parent, RunStep *product) const = 0;
    virtual RunStep *clone(RunStepList *parent, RunStep *product) = 0;
};

class RunStepList : public ProjectExplorer::ProjectConfiguration
{
    Q_OBJECT

public:
    RunStepList(QObject *parent, Core::Id id);
    RunStepList(QObject *parent, RunStepList *source);
    RunStepList(QObject *parent, const QVariantMap &data);
    ~RunStepList() override;

    QList<RunStep *> steps() const;
    bool isNull() const;
    int count() const;
    bool isEmpty() const;
    bool contains(Core::Id id) const;
    bool enabled() const;

    void insertStep(int position, RunStep *step);
    void appendStep(RunStep *step) { insertStep(count(), step); }
    bool removeStep(int position);
    void moveStepUp(int position);
    RunStep *at(int position);

    ProjectExplorer::Target *target() const;

    virtual QVariantMap toMap() const override;
    void cloneSteps(RunStepList *source);

signals:
    void stepInserted(int position);
    void aboutToRemoveStep(int position);
    void stepRemoved(int position);
    void stepMoved(int from, int to);

protected:
    virtual bool fromMap(const QVariantMap &map) override;

private:
    QList<RunStep *> m_steps;
    bool m_isNull;
};

class RunStepConfigWidget : public QWidget
{
    Q_OBJECT
public:
    virtual QString summaryText() const = 0;
    virtual QString additionalSummaryText() const { return QString(); }
    virtual QString displayName() const = 0;
    virtual bool showWidget() const { return true; }

signals:
    void updateSummary();
    void updateAdditionalSummary();
};

} // Internal
} // ROSProjectManager

#endif // RUNSTEP_H
