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

#include "ros_run_step.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>




static const char runStepEnabledKey[] = "ProjectExplorer.RunStep.Enabled";



namespace ROSProjectManager {
namespace Internal {

namespace {

IRunStepFactory *findCloneFactory(RunStepList *parent, RunStep *source)
{
    return ExtensionSystem::PluginManager::getObject<IRunStepFactory>(
        [&parent, &source](IRunStepFactory *factory) {
            return factory->canClone(parent, source);
        });
}

IRunStepFactory *findRestoreFactory(RunStepList *parent, const QVariantMap &map)
{
    return ExtensionSystem::PluginManager::getObject<IRunStepFactory>(
        [&parent, &map](IRunStepFactory *factory) {
            return factory->canRestore(parent, map);
        });
}

const char STEPS_COUNT_KEY[] = "ProjectExplorer.RunStepList.StepsCount";
const char STEPS_PREFIX[] = "ProjectExplorer.RunStepList.Step.";

} // namespace


RunStep::RunStep(RunStepList *rsl, Core::Id id) :
    ProjectConfiguration(rsl, id), m_enabled(true)
{
    Q_ASSERT(rsl);
}

RunStep::RunStep(RunStepList *rsl, RunStep *rs) :
    ProjectConfiguration(rsl, rs), m_enabled(rs->m_enabled)
{
    Q_ASSERT(rsl);
    setDisplayName(rs->displayName());
}

bool RunStep::fromMap(const QVariantMap &map)
{
    m_enabled = map.value(QLatin1String(runStepEnabledKey), true).toBool();
    return ProjectConfiguration::fromMap(map);
}

QVariantMap RunStep::toMap() const
{
    QVariantMap map = ProjectConfiguration::toMap();
    map.insert(QLatin1String(runStepEnabledKey), m_enabled);
    return map;
}

ProjectExplorer::RunConfiguration *RunStep::runConfiguration() const
{
    return qobject_cast<ProjectExplorer::RunConfiguration *>(parent()->parent());
}

ProjectExplorer::ProjectConfiguration *RunStep::projectConfiguration() const
{
    return static_cast<ProjectConfiguration *>(parent()->parent());
}

ProjectExplorer::Target *RunStep::target() const
{
    return qobject_cast<ProjectExplorer::Target *>(parent()->parent()->parent());
}

ProjectExplorer::Project *RunStep::project() const
{
    return target()->project();
}

/*!
    If this function returns \c true, the user cannot delete this build step for
    this target and the user is prevented from changing the order in which
    immutable steps are run. The default implementation returns \c false.
*/

bool RunStep::immutable() const
{
    return false;
}

bool RunStep::runInGuiThread() const
{
    return false;
}

/*!
    This function needs to be reimplemented only for build steps that return
    \c false from runInGuiThread().

    \sa runInGuiThread()
*/
void RunStep::cancel()
{
    // Do nothing
}

void RunStep::setEnabled(bool b)
{
    if (m_enabled == b)
        return;
    m_enabled = b;
    emit enabledChanged();
}

bool RunStep::enabled() const
{
    return m_enabled;
}

IRunStepFactory::IRunStepFactory(QObject *parent) :
    QObject(parent)
{ }


RunStepList::RunStepList(QObject *parent, Core::Id id) :
    ProjectConfiguration(parent, id),
    m_isNull(false)
{
    Q_ASSERT(parent);
}

RunStepList::RunStepList(QObject *parent, RunStepList *source) :
    ProjectConfiguration(parent, source),
    m_isNull(source->m_isNull)
{
    setDisplayName(source->displayName());
    Q_ASSERT(parent);
    // do not clone the steps here:
    // The BC is not fully set up yet and thus some of the buildstepfactories
    // will fail to clone the buildsteps!
}

RunStepList::RunStepList(QObject *parent, const QVariantMap &data) :
    ProjectConfiguration(parent, Core::Id())
{
    Q_ASSERT(parent);
    m_isNull = !fromMap(data);
}

RunStepList::~RunStepList()
{
    qDeleteAll(m_steps);
}

QVariantMap RunStepList::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    // Save build steps
    map.insert(QString::fromLatin1(STEPS_COUNT_KEY), m_steps.count());
    for (int i = 0; i < m_steps.count(); ++i)
        map.insert(QString::fromLatin1(STEPS_PREFIX) + QString::number(i), m_steps.at(i)->toMap());

    return map;
}

bool RunStepList::isNull() const
{
    return m_isNull;
}

int RunStepList::count() const
{
    return m_steps.count();
}

bool RunStepList::isEmpty() const
{
    return m_steps.isEmpty();
}

bool RunStepList::contains(Core::Id id) const
{
    return Utils::anyOf(steps(), [id](RunStep *bs){
        return bs->id() == id;
    });
}

void RunStepList::cloneSteps(RunStepList *source)
{
    Q_ASSERT(source);
    foreach (RunStep *originalbs, source->steps()) {
        IRunStepFactory *factory(findCloneFactory(this, originalbs));
        if (!factory)
            continue;
        RunStep *clonebs(factory->clone(this, originalbs));
        if (clonebs)
            m_steps.append(clonebs);
    }
}

bool RunStepList::fromMap(const QVariantMap &map)
{
    // We need the ID set before trying to restore the steps!
    if (!ProjectConfiguration::fromMap(map))
        return false;

    int maxSteps = map.value(QString::fromLatin1(STEPS_COUNT_KEY), 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        QVariantMap bsData(map.value(QString::fromLatin1(STEPS_PREFIX) + QString::number(i)).toMap());
        if (bsData.isEmpty()) {
            qWarning() << "No step data found for" << i << "(continuing).";
            continue;
        }
        IRunStepFactory *factory = findRestoreFactory(this, bsData);
        if (!factory) {
            qWarning() << "No factory for step" << i << "in list" << displayName() << "found (continuing).";
            continue;
        }
        RunStep *bs(factory->restore(this, bsData));
        if (!bs) {
            qWarning() << "Restoration of step" << i << "failed (continuing).";
            continue;
        }
        insertStep(m_steps.count(), bs);
    }
    return true;
}

QList<RunStep *> RunStepList::steps() const
{
    return m_steps;
}

void RunStepList::insertStep(int position, RunStep *step)
{
    m_steps.insert(position, step);
    emit stepInserted(position);
}

bool RunStepList::removeStep(int position)
{
    RunStep *bs = at(position);
//    if (BuildManager::isBuilding(bs))
//        return false;

    emit aboutToRemoveStep(position);
    m_steps.removeAt(position);
    delete bs;
    emit stepRemoved(position);
    return true;
}

void RunStepList::moveStepUp(int position)
{
    m_steps.swap(position - 1, position);
    emit stepMoved(position, position - 1);
}

RunStep *RunStepList::at(int position)
{
    return m_steps.at(position);
}

ProjectExplorer::Target *RunStepList::target() const
{
    Q_ASSERT(parent());
    ProjectExplorer::RunConfiguration *rc = qobject_cast<ProjectExplorer::RunConfiguration *>(parent());
    if (rc)
      return rc->target();

    return 0;
}


} // namespace Internal
} // namespace ROSProjectManager
