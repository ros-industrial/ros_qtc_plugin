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
#include "ros_run_step.h"
#include "ros_project_constants.h"

#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <QDebug>

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
    ProjectConfiguration(rsl), m_enabled(true)
{
    initialize(id);
    Q_ASSERT(rsl);

}

RunStep::RunStep(RunStepList *rsl, RunStep *rs) :
    ProjectConfiguration(rsl), m_enabled(rs->m_enabled)
{
    copyFrom(rs);
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

bool RunStep::isActive() const
{
    return projectConfiguration()->isActive();
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
    ProjectConfiguration(parent)
{
    Q_ASSERT(parent);
    initialize(id);
}

RunStepList::RunStepList(QObject *parent, RunStepList *source) :
    ProjectConfiguration(parent)
{
    copyFrom(source);
    setDisplayName(source->displayName());
    Q_ASSERT(parent);
    // do not clone the steps here:
    // The BC is not fully set up yet and thus some of the buildstepfactories
    // will fail to clone the buildsteps!
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

bool RunStepList::enabled() const
{
    foreach(RunStep *rs, m_steps)
    {
        if (rs->enabled() == true)
            return true;
    }
    return false;
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

bool RunStepList::isActive() const
{
    return qobject_cast<ProjectConfiguration *>(parent())->isActive();
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

void RunStepList::runStep_enabledChanged(RunStep *step)
{
    if (step->enabled() == true)
    {
        foreach(RunStep *rs, m_steps)
        {
            if (rs->enabled() == true && rs->id() == Constants::ROS_ATTACH_TO_NODE_ID && rs != step)
            {
                rs->setEnabled(false);
            }
        }
    }
}

QList<RunStep *> RunStepList::steps() const
{
    return m_steps;
}

void RunStepList::insertStep(int position, RunStep *step)
{
    if (step->id() == Constants::ROS_ATTACH_TO_NODE_ID)
    {
        if (step->enabled() == true)
        {
            foreach(RunStep *rs, m_steps)
            {
                if (rs->enabled() == true && rs->id() == Constants::ROS_ATTACH_TO_NODE_ID)
                {
                    rs->setEnabled(false);
                }
            }
        }
        connect(step, &RunStep::enabledChanged,
                this, [this, step] () {runStep_enabledChanged(step);});
    }
    m_steps.insert(position, step);
    emit stepInserted(position);
}

bool RunStepList::removeStep(int position)
{
    RunStep *bs = at(position);

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

ProjectExplorer::Project *RunStepList::project() const
{
    return target()->project();
}

} // namespace Internal
} // namespace ROSProjectManager
