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

    bool m_runStepEnabled;
    Utils::FadingWidget *m_firstWidget;
    Utils::FadingWidget *m_secondWidget;
    qreal m_targetOpacity;
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

    void init(RunStepList *bsl);

signals:
    void runStepListChanged();

private:
    void updateAddRunStepMenu();
    void addRunStep(int pos);
    void updateSummary();
    void updateAdditionalSummary();
    void updateEnabledState();
    void triggerStepMoveUp(int pos);
    void stepMoved(int from, int to);
    void triggerStepMoveDown(int pos);
    void triggerRemoveRunStep(int pos);
    void removeRunStep(int pos);
    void triggerDisable(int pos);

    void setupUi();
    void updateRunStepButtonsState();
    void addRunStepWidget(int pos, RunStep *step);

    RunStepList *m_runStepList;

    QList<RunStepsWidgetData *> m_runStepsData;

    QVBoxLayout *m_vbox;

    QLabel *m_noStepsLabel;
    QPushButton *m_addButton;

    QSignalMapper *m_disableMapper;
    QSignalMapper *m_upMapper;
    QSignalMapper *m_downMapper;
    QSignalMapper *m_removeMapper;

    int m_leftMargin;
};

namespace Ui { class RunStepsPage; }

class RunStepsPage : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    RunStepsPage(ROSRunConfiguration *rc);
    virtual ~RunStepsPage();

private:
    RunStepListWidget *m_widget;
};

} // Internal
} // ROSProjectManager

#endif // ROSRUNSTEPSPAGE_H
