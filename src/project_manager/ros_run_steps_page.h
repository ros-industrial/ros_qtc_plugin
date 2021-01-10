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
#ifndef ROSRUNSTEPSPAGE_H
#define ROSRUNSTEPSPAGE_H

#include "ros_run_step.h"
#include "ros_run_configuration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/namedwidget.h>
#include <utils/detailsbutton.h>

QT_BEGIN_NAMESPACE
class QPushButton;
class QToolButton;
class QLabel;
class QVBoxLayout;
class QSignalMapper;
class QGraphicsOpacityEffect;
QT_END_NAMESPACE

namespace Utils { class DetailsWidget; }

namespace ProjectExplorer { class Target; }

namespace ROSProjectManager {
namespace Internal {

class ToolWidget : public Utils::FadingPanel
{
    Q_OBJECT
public:
    ToolWidget(QWidget *parent = 0);

    void fadeTo(qreal value);
    void setOpacity(qreal value);

    void setRunStepEnabled(bool b);
    void setUpEnabled(bool b);
    void setDownEnabled(bool b);
    void setRemoveEnabled(bool b);
    void setUpVisible(bool b);
    void setDownVisible(bool b);

signals:
    void disabledClicked();
    void upClicked();
    void downClicked();
    void removeClicked();

private:
    QToolButton *m_disableButton;
    QToolButton *m_upButton;
    QToolButton *m_downButton;
    QToolButton *m_removeButton;

    bool m_runStepEnabled = true;
    Utils::FadingWidget *m_firstWidget;
    Utils::FadingWidget *m_secondWidget;
    qreal m_targetOpacity = 0.999;
};

class RunStepsWidgetData
{
public:
    RunStepsWidgetData(RunStep *s);
    ~RunStepsWidgetData();

    RunStep *step;
    RunStepConfigWidget *widget;
    Utils::DetailsWidget *detailsWidget;
    ToolWidget *toolWidget;
};

class RunStepListWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    RunStepListWidget(QWidget *parent = 0);
    virtual ~RunStepListWidget();

    void init(RunStepList *rsl);

private:
    void updateAddRunStepMenu();
    void addRunStep(int pos);
    void updateSummary();
    void updateAdditionalSummary();
    void updateEnabledState();
    void stepMoved(int from, int to);
    void removeRunStep(int pos);

    void setupUi();
    void updateRunStepButtonsState();
    void addRunStepWidget(int pos, RunStep *step);

    RunStepList *m_runStepList = nullptr;

    QList<RunStepsWidgetData *> m_runStepsData;

    QVBoxLayout *m_vbox = nullptr;

    QLabel *m_noStepsLabel = nullptr;
    QPushButton *m_addButton = nullptr;
};

namespace Ui { class RunStepsPage; }

class RunStepsPage : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    RunStepsPage(ROSRunConfiguration *rc, Utils::Id id);

private:
    Utils::Id m_id;
    RunStepListWidget *m_widget = nullptr;
};

} // Internal
} // ROSProjectManager

#endif // ROSRUNSTEPSPAGE_H
