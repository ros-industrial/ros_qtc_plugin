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

#include <coreplugin/messagemanager.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/project.h>
#include <utils/algorithm.h>
#include <QDebug>

static const char runStepEnabledKey[] = "ProjectExplorer.RunStep.Enabled";



namespace ROSProjectManager {
namespace Internal {

static QList<RunStepFactory *> g_runStepFactories;

RunStepFactory::RunStepFactory()
{
    g_runStepFactories.append(this);
}

RunStepFactory::~RunStepFactory()
{
    g_runStepFactories.removeOne(this);
}

const QList<RunStepFactory *> RunStepFactory::allRunStepFactories()
{
    return g_runStepFactories;
}

bool RunStepFactory::canHandle(RunStepList *rsl) const
{
    if (!m_supportedStepLists.isEmpty() && !m_supportedStepLists.contains(rsl->id()))
        return false;

    auto config = qobject_cast<ProjectExplorer::ProjectConfiguration *>(rsl->parent());

    if (!m_supportedDeviceTypes.isEmpty()) {
        ProjectExplorer::Target *target = rsl->target();
        QTC_ASSERT(target, return false);
        Utils::Id deviceType = ProjectExplorer::DeviceTypeKitAspect::deviceTypeId(target->kit());
        if (!m_supportedDeviceTypes.contains(deviceType))
            return false;
    }

    if (m_supportedProjectType.isValid()) {
        if (!config)
            return false;
        Utils::Id projectId = config->project()->id();
        if (projectId != m_supportedProjectType)
            return false;
    }

    if (!m_isRepeatable && rsl->contains(m_info.id))
        return false;

    if (m_supportedConfiguration.isValid()) {
        if (!config)
            return false;
        Utils::Id configId = config->id();
        if (configId != m_supportedConfiguration)
            return false;
    }

    return true;
}

void RunStepFactory::setDisplayName(const QString &displayName)
{
    m_info.displayName = displayName;
}

void RunStepFactory::setFlags(RunStepInfo::Flags flags)
{
    m_info.flags = flags;
}

void RunStepFactory::setSupportedStepList(Utils::Id id)
{
    m_supportedStepLists = {id};
}

void RunStepFactory::setSupportedStepLists(const QList<Utils::Id> &ids)
{
    m_supportedStepLists = ids;
}

void RunStepFactory::setSupportedConfiguration(Utils::Id id)
{
    m_supportedConfiguration = id;
}

void RunStepFactory::setSupportedProjectType(Utils::Id id)
{
    m_supportedProjectType = id;
}

void RunStepFactory::setSupportedDeviceType(Utils::Id id)
{
    m_supportedDeviceTypes = {id};
}

void RunStepFactory::setSupportedDeviceTypes(const QList<Utils::Id> &ids)
{
    m_supportedDeviceTypes = ids;
}

RunStepInfo RunStepFactory::stepInfo() const
{
    return m_info;
}

Utils::Id RunStepFactory::stepId() const
{
    return m_info.id;
}

RunStep *RunStepFactory::create(RunStepList *parent, Utils::Id id)
{
    RunStep *rs = nullptr;
    if (id == m_info.id)
        rs = m_info.creator(parent);
    return rs;
}

RunStep *RunStepFactory::restore(RunStepList *parent, const Utils::Store &map)
{
    RunStep *rs = m_info.creator(parent);
    if (!rs)
        return nullptr;
    rs->fromMap(map);
    return rs;
}

namespace {
const char STEPS_COUNT_KEY[] = "ProjectExplorer.RunStepList.StepsCount";
const char STEPS_PREFIX[] = "ProjectExplorer.RunStepList.Step.";
}


RunStep::RunStep(RunStepList *rsl, Utils::Id id) :
    ProjectConfiguration(rsl->target(), id)
{
    //
}

void RunStep::fromMap(const Utils::Store &map)
{
    m_enabled = map.value(runStepEnabledKey, true).toBool();
    ProjectConfiguration::fromMap(map);
}

void RunStep::toMap(Utils::Store &map) const
{
    ProjectConfiguration::toMap(map);
    map.insert(runStepEnabledKey, m_enabled);
}

ProjectExplorer::RunConfiguration *RunStep::runConfiguration() const
{
    auto config = qobject_cast<ProjectExplorer::RunConfiguration *>(parent()->parent());
    if (config)
      return config;

    return target()->activeRunConfiguration();
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

RunStepList::RunStepList(ProjectExplorer::Target *target, Utils::Id id) :
    ProjectConfiguration(target, id)
{
    Q_ASSERT(target);
    setDefaultDisplayName(tr("Run"));
}


RunStepList::~RunStepList()
{
    clear();
}

void RunStepList::clear()
{
    qDeleteAll(m_steps);
    m_steps.clear();
}

void RunStepList::toMap(Utils::Store &map) const
{
    ProjectConfiguration::toMap(map);
    // Save run steps
    map.insert(STEPS_COUNT_KEY, m_steps.count());
    for (int i = 0; i < m_steps.count(); ++i) {
        const Utils::Key key_step = (QString(STEPS_PREFIX) + QString::number(i)).toUtf8();
        Utils::Store map_step;
        m_steps.at(i)->toMap(map_step);
        map.insert(key_step, Utils::variantFromStore(map_step));
    }

}

int RunStepList::count() const
{
    return m_steps.count();
}

bool RunStepList::isEmpty() const
{
    return m_steps.isEmpty();
}

bool RunStepList::contains(Utils::Id id) const
{
    return Utils::anyOf(steps(), [id](RunStep *rs){
        return rs->id() == id;
    });
}

bool RunStepList::enabled() const
{
    for (RunStep *rs : m_steps) {
        if (rs->enabled() == true)
            return true;
    }
    return false;
}

void RunStepList::fromMap(const Utils::Store &map)
{
    clear();

    // We need the ID set before trying to restore the steps!
    ProjectConfiguration::fromMap(map);

    const QList<RunStepFactory *> factories = RunStepFactory::allRunStepFactories();
    int maxSteps = map.value(STEPS_COUNT_KEY, 0).toInt();
    for (int i = 0; i < maxSteps; ++i) {
        const Utils::Key key_step = (QString(STEPS_PREFIX) + QString::number(i)).toUtf8();
        Utils::Store rsData = Utils::storeFromVariant(map.value(key_step).toMap());
        if (rsData.isEmpty()) {
            Core::MessageManager::writeSilently(tr("[ROS Warning] No step data found for step %1 (continuing).").arg(i));
            continue;
        }
        bool handled = false;
        Utils::Id stepId = ProjectExplorer::idFromMap(rsData);
        for (RunStepFactory *factory : factories) {
            if (factory->stepId() == stepId) {
                if (factory->canHandle(this)) {
                    if (RunStep *rs = factory->restore(this, rsData)) {
                        appendStep(rs);
                        handled = true;
                    } else {
                        Core::MessageManager::writeSilently(tr("[ROS Warning] Restoration of step %1 failed (continuing).").arg(i));
                    }
                }
            }
        }
        QTC_ASSERT(handled, qDebug() << "No factory for run step" << stepId.toString() << "found.");
    }
}

void RunStepList::runStep_enabledChanged(RunStep *step)
{
    if (step->enabled() == true)
    {
        for (RunStep *rs : m_steps)
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

QList<RunStep *> RunStepList::steps(const std::function<bool (const RunStep *)> &filter) const
{
    return Utils::filtered(steps(), filter);
}

void RunStepList::insertStep(int position, RunStep *step)
{
    if (step->id() == Constants::ROS_ATTACH_TO_NODE_ID)
    {
        if (step->enabled() == true)
        {
            for (RunStep *rs : m_steps)
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
    RunStep *rs = at(position);

    emit aboutToRemoveStep(position);
    m_steps.removeAt(position);
    delete rs;
    emit stepRemoved(position);
    return true;
}

void RunStepList::moveStepUp(int position)
{
    m_steps.swapItemsAt(position - 1, position);
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
