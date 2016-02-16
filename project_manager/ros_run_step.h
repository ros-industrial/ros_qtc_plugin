/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/
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

    virtual void run(QFutureInterface<bool> &fi) = 0;

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

//    enum OutputFormat { NormalOutput, ErrorOutput, MessageOutput, ErrorMessageOutput };
//    enum OutputNewlineSetting { DoAppendNewline, DontAppendNewline };

signals:
    /// Adds a \p task to the Issues pane.
    /// Do note that for linking compile output with tasks, you should first emit the task
    /// and then emit the output. \p linkedOutput lines will be linked. And the last \p skipLines will
    /// be skipped.
    void addTask(const ProjectExplorer::Task &task, int linkedOutputLines = 0, int skipLines = 0);

//    /// Adds \p string to the compile output view, formatted in \p format
//    void addOutput(const QString &string, ProjectExplorer::BuildStep::OutputFormat format,
//        ProjectExplorer::BuildStep::OutputNewlineSetting newlineSetting = DoAppendNewline);

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
