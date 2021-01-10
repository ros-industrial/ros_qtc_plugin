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
    friend class RunStepFactory;
    explicit RunStep(RunStepList *rsl, Utils::Id id);

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
    bool m_enabled = true;
};

class RunStepInfo
{
public:
    enum Flags {
        Uncreatable = 1 << 0,
        Unclonable  = 1 << 1,
        UniqueStep  = 1 << 8    // Can't be used twice in a RunStepList
    };

    using RunStepCreator = std::function<RunStep *(RunStepList *)>;

    Utils::Id id;
    QString displayName;
    Flags flags = Flags();
    RunStepCreator creator;
};

class RunStepFactory
{
public:
    RunStepFactory();
    virtual ~RunStepFactory();

    static const QList<RunStepFactory *> allRunStepFactories();

    RunStepInfo stepInfo() const;
    Utils::Id stepId() const;
    RunStep *create(RunStepList *parent, Utils::Id id);
    RunStep *restore(RunStepList *parent, const QVariantMap &map);

    bool canHandle(RunStepList *rsl) const;

protected:
    RunStepFactory(const RunStepFactory &) = delete;
    RunStepFactory &operator=(const RunStepFactory &) = delete;

    template <class RunStepType>
    void registerStep(Utils::Id id)
    {
        QTC_CHECK(!m_info.creator);
        m_info.id = id;
        m_info.creator = [](RunStepList *rsl) { return new RunStepType(rsl); };
    }

    void setSupportedStepList(Utils::Id id);
    void setSupportedStepLists(const QList<Utils::Id> &ids);
    void setSupportedConfiguration(Utils::Id id);
    void setSupportedProjectType(Utils::Id id);
    void setSupportedDeviceType(Utils::Id id);
    void setSupportedDeviceTypes(const QList<Utils::Id> &ids);
    void setRepeatable(bool on) { m_isRepeatable = on; }
    void setDisplayName(const QString &displayName);
    void setFlags(RunStepInfo::Flags flags);

private:
    RunStepInfo m_info;

    Utils::Id m_supportedProjectType;
    QList<Utils::Id> m_supportedDeviceTypes;
    QList<Utils::Id> m_supportedStepLists;
    Utils::Id m_supportedConfiguration;
    bool m_isRepeatable = true;
};

class RunStepList : public ProjectExplorer::ProjectConfiguration
{
    Q_OBJECT

public:
    RunStepList(QObject *parent, Utils::Id id);
    ~RunStepList() override;

    void clear();

    QList<RunStep *> steps() const;
    QList<RunStep *> steps(const std::function<bool(const RunStep *)> &filter) const;
        template <class RS> RS *firstOfType() {
            RS *rs = nullptr;
            for (int i = 0; i < count(); ++i) {
                rs = qobject_cast<RS *>(at(i));
                if (rs)
                    return rs;
            }
            return nullptr;
        }
        template <class RS> QList<RS *>allOfType() {
            QList<RS *> result;
            RS *rs = nullptr;
            for (int i = 0; i < count(); ++i) {
                rs = qobject_cast<RS *>(at(i));
                if (rs)
                    result.append(rs);
            }
            return result;
    }
    int count() const;
    bool isEmpty() const;
    bool contains(Utils::Id id) const;
    bool enabled() const;

    void insertStep(int position, RunStep *step);
    void appendStep(RunStep *step) { insertStep(count(), step); }
    bool removeStep(int position);
    void moveStepUp(int position);
    RunStep *at(int position);

    ProjectExplorer::Target *target() const;

    virtual QVariantMap toMap() const override;
    bool fromMap(const QVariantMap &map) override;

signals:
    void stepInserted(int position);
    void aboutToRemoveStep(int position);
    void stepRemoved(int position);
    void stepMoved(int from, int to);

private slots:
    void runStep_enabledChanged(RunStep *step);

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
    virtual bool showWidget() const { return m_showWidget; }
    void setShowWidget(bool showWidget) { m_showWidget = showWidget; }

signals:
    void updateSummary();
    void updateAdditionalSummary();

private:
    bool m_showWidget = true;
};

} // Internal
} // ROSProjectManager

#endif // RUNSTEP_H
