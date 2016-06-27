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
