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

#include "ros_run_steps_page.h"


#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorericons.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreicons.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>
#include <utils/detailswidget.h>
#include <utils/hostosinfo.h>
#include <utils/theme/theme.h>

#include <QSignalMapper>

#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QMessageBox>

namespace ROSProjectManager {
namespace Internal {

ToolWidget::ToolWidget(QWidget *parent)
    : FadingPanel(parent), m_runStepEnabled(true), m_targetOpacity(1.0f)
{
    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(4);
    layout->setSpacing(4);
    setLayout(layout);
    m_firstWidget = new Utils::FadingWidget(this);
    m_firstWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(0, 0, 0, 0);
    hbox->setSpacing(0);
    m_firstWidget->setLayout(hbox);
    QSize buttonSize(20, Utils::HostOsInfo::isMacHost() ? 20 : 26);

    m_disableButton = new QToolButton(m_firstWidget);
    m_disableButton->setAutoRaise(true);
    m_disableButton->setToolTip(RunStepListWidget::tr("Disable"));
    m_disableButton->setFixedSize(buttonSize);
    m_disableButton->setIcon(ProjectExplorer::Icons::BUILDSTEP_DISABLE.icon());
    m_disableButton->setCheckable(true);
    hbox->addWidget(m_disableButton);
    layout->addWidget(m_firstWidget);

    m_secondWidget = new Utils::FadingWidget(this);
    m_secondWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(4);
    m_secondWidget->setLayout(hbox);

    m_upButton = new QToolButton(m_secondWidget);
    m_upButton->setAutoRaise(true);
    m_upButton->setToolTip(RunStepListWidget::tr("Move Up"));
    m_upButton->setFixedSize(buttonSize);
    m_upButton->setIcon(ProjectExplorer::Icons::BUILDSTEP_MOVEUP.icon());
    hbox->addWidget(m_upButton);

    m_downButton = new QToolButton(m_secondWidget);
    m_downButton->setAutoRaise(true);
    m_downButton->setToolTip(RunStepListWidget::tr("Move Down"));
    m_downButton->setFixedSize(buttonSize);
    m_downButton->setIcon(ProjectExplorer::Icons::BUILDSTEP_MOVEDOWN.icon());
    hbox->addWidget(m_downButton);

    m_removeButton  = new QToolButton(m_secondWidget);
    m_removeButton->setAutoRaise(true);
    m_removeButton->setToolTip(RunStepListWidget::tr("Remove Item"));
    m_removeButton->setFixedSize(buttonSize);
    m_removeButton->setIcon(ProjectExplorer::Icons::BUILDSTEP_REMOVE.icon());
    hbox->addWidget(m_removeButton);

    layout->addWidget(m_secondWidget);

    connect(m_disableButton, &QAbstractButton::clicked, this, &ToolWidget::disabledClicked);
    connect(m_upButton, &QAbstractButton::clicked, this, &ToolWidget::upClicked);
    connect(m_downButton, &QAbstractButton::clicked, this, &ToolWidget::downClicked);
    connect(m_removeButton, &QAbstractButton::clicked, this, &ToolWidget::removeClicked);
}

void ToolWidget::setOpacity(qreal value)
{
    m_targetOpacity = value;
    if (m_runStepEnabled)
        m_firstWidget->setOpacity(value);
    m_secondWidget->setOpacity(value);
}

void ToolWidget::fadeTo(qreal value)
{
    m_targetOpacity = value;
    if (m_runStepEnabled)
        m_firstWidget->fadeTo(value);
    m_secondWidget->fadeTo(value);
}

void ToolWidget::setRunStepEnabled(bool b)
{
    m_runStepEnabled = b;
    if (m_runStepEnabled) {
        if (Utils::HostOsInfo::isMacHost())
            m_firstWidget->setOpacity(m_targetOpacity);
        else
            m_firstWidget->fadeTo(m_targetOpacity);
    } else {
        if (Utils::HostOsInfo::isMacHost())
            m_firstWidget->setOpacity(1.0);
        else
            m_firstWidget->fadeTo(1.0);
    }
    m_disableButton->setChecked(!b);
}

void ToolWidget::setUpEnabled(bool b)
{
    m_upButton->setEnabled(b);
}

void ToolWidget::setDownEnabled(bool b)
{
    m_downButton->setEnabled(b);
}

void ToolWidget::setRemoveEnabled(bool b)
{
    m_removeButton->setEnabled(b);
}

void ToolWidget::setUpVisible(bool b)
{
    m_upButton->setVisible(b);
}

void ToolWidget::setDownVisible(bool b)
{
    m_downButton->setVisible(b);
}

RunStepsWidgetData::RunStepsWidgetData(RunStep *s) :
    step(s), widget(0), detailsWidget(0)
{
    widget = s->createConfigWidget();
    Q_ASSERT(widget);

    detailsWidget = new Utils::DetailsWidget;
    detailsWidget->setWidget(widget);

    toolWidget = new ToolWidget(detailsWidget);
    toolWidget->setRunStepEnabled(step->enabled());

    detailsWidget->setToolWidget(toolWidget);
    detailsWidget->setContentsMargins(0, 0, 0, 1);
    detailsWidget->setSummaryText(widget->summaryText());
    detailsWidget->setAdditionalSummaryText(widget->additionalSummaryText());
}

RunStepsWidgetData::~RunStepsWidgetData()
{
    delete detailsWidget; // other widgets are children of that!
    // We do not own the step
}

RunStepListWidget::RunStepListWidget(QWidget *parent) :
    NamedWidget(parent),
    m_runStepList(0),
    m_addButton(0)
{
}

RunStepListWidget::~RunStepListWidget()
{
    qDeleteAll(m_runStepsData);
    m_runStepsData.clear();
}

void RunStepListWidget::updateSummary()
{
    RunStepConfigWidget *widget = qobject_cast<RunStepConfigWidget *>(sender());
    if (widget) {
        foreach (const RunStepsWidgetData *s, m_runStepsData) {
            if (s->widget == widget) {
                s->detailsWidget->setSummaryText(widget->summaryText());
                break;
            }
        }
    }
}

void RunStepListWidget::updateAdditionalSummary()
{
    RunStepConfigWidget *widget = qobject_cast<RunStepConfigWidget *>(sender());
    if (widget) {
        foreach (const RunStepsWidgetData *s, m_runStepsData) {
            if (s->widget == widget) {
                s->detailsWidget->setAdditionalSummaryText(widget->additionalSummaryText());
                break;
            }
        }
    }
}

void RunStepListWidget::updateEnabledState()
{
    RunStep *step = qobject_cast<RunStep *>(sender());
    if (step) {
        foreach (const RunStepsWidgetData *s, m_runStepsData) {
            if (s->step == step) {
                s->toolWidget->setRunStepEnabled(step->enabled());
                break;
            }
        }
    }
}

void RunStepListWidget::init(RunStepList *bsl)
{
    Q_ASSERT(bsl);
    if (bsl == m_runStepList)
        return;

    setupUi();

    if (m_runStepList) {
        disconnect(m_runStepList, &RunStepList::stepInserted,
                   this, &RunStepListWidget::addRunStep);
        disconnect(m_runStepList, &RunStepList::stepRemoved,
                   this, &RunStepListWidget::removeRunStep);
        disconnect(m_runStepList, &RunStepList::stepMoved,
                   this, &RunStepListWidget::stepMoved);
    }

    connect(bsl, &RunStepList::stepInserted, this, &RunStepListWidget::addRunStep);
    connect(bsl, &RunStepList::stepRemoved, this, &RunStepListWidget::removeRunStep);
    connect(bsl, &RunStepList::stepMoved, this, &RunStepListWidget::stepMoved);

    qDeleteAll(m_runStepsData);
    m_runStepsData.clear();

    m_runStepList = bsl;
    //: %1 is the name returned by BuildStepList::displayName
    setDisplayName(tr("%1 Steps").arg(m_runStepList->displayName()));

    for (int i = 0; i < bsl->count(); ++i) {
        addRunStep(i);
        // addBuilStep expands the config widget by default, which we don't want here
        if (m_runStepsData.at(i)->widget->showWidget())
            m_runStepsData.at(i)->detailsWidget->setState(Utils::DetailsWidget::Collapsed);
    }

    m_noStepsLabel->setVisible(bsl->isEmpty());
    m_noStepsLabel->setText(tr("No %1 Steps").arg(m_runStepList->displayName()));

    m_addButton->setText(tr("Add %1 Step").arg(m_runStepList->displayName()));

    updateRunStepButtonsState();
}

void RunStepListWidget::updateAddRunStepMenu()
{
    QMap<QString, QPair<Core::Id, IRunStepFactory *> > map;
    //Build up a list of possible steps and save map the display names to the (internal) name and factories.
    QList<IRunStepFactory *> factories = ExtensionSystem::PluginManager::getObjects<IRunStepFactory>();
    foreach (IRunStepFactory *factory, factories) {
        QList<Core::Id> ids = factory->availableCreationIds(m_runStepList);
        foreach (Core::Id id, ids)
            map.insert(factory->displayNameForId(id), QPair<Core::Id, IRunStepFactory *>(id, factory));
    }

    // Ask the user which one to add
    QMenu *menu = m_addButton->menu();
    menu->clear();
    if (!map.isEmpty()) {
        QMap<QString, QPair<Core::Id, IRunStepFactory *> >::const_iterator it, end;
        end = map.constEnd();
        for (it = map.constBegin(); it != end; ++it) {
            QAction *action = menu->addAction(it.key());
            IRunStepFactory *factory = it.value().second;
            Core::Id id = it.value().first;

            connect(action, &QAction::triggered, [id, factory, this]() {
                RunStep *newStep = factory->create(m_runStepList, id);
                QTC_ASSERT(newStep, return);
                int pos = m_runStepList->count();
                m_runStepList->insertStep(pos, newStep);
            });
        }
    }
}

void RunStepListWidget::addRunStepWidget(int pos, RunStep *step)
{
    // create everything
    RunStepsWidgetData *s = new RunStepsWidgetData(step);
    m_runStepsData.insert(pos, s);

    m_vbox->insertWidget(pos, s->detailsWidget);

    connect(s->widget, &RunStepConfigWidget::updateSummary,
            this, &RunStepListWidget::updateSummary);
    connect(s->widget, &RunStepConfigWidget::updateAdditionalSummary,
            this, &RunStepListWidget::updateAdditionalSummary);

    connect(s->step, &RunStep::enabledChanged,
            this, &RunStepListWidget::updateEnabledState);

    connect(s->toolWidget, &ToolWidget::disabledClicked,
            m_disableMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    connect(s->toolWidget, &ToolWidget::upClicked,
            m_upMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    connect(s->toolWidget, &ToolWidget::downClicked,
            m_downMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    connect(s->toolWidget, &ToolWidget::removeClicked,
            m_removeMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
}

void RunStepListWidget::addRunStep(int pos)
{
    RunStep *newStep = m_runStepList->at(pos);
    addRunStepWidget(pos, newStep);
    RunStepsWidgetData *s = m_runStepsData.at(pos);
    // Expand new build steps by default
    if (s->widget->showWidget())
        s->detailsWidget->setState(Utils::DetailsWidget::Expanded);
    else
        s->detailsWidget->setState(Utils::DetailsWidget::OnlySummary);

    m_noStepsLabel->setVisible(false);
    updateRunStepButtonsState();
}

void RunStepListWidget::triggerStepMoveUp(int pos)
{
    m_runStepList->moveStepUp(pos);
}

void RunStepListWidget::stepMoved(int from, int to)
{
    m_vbox->insertWidget(to, m_runStepsData.at(from)->detailsWidget);

    RunStepsWidgetData *data = m_runStepsData.at(from);
    m_runStepsData.removeAt(from);
    m_runStepsData.insert(to, data);

    updateRunStepButtonsState();
}

void RunStepListWidget::triggerStepMoveDown(int pos)
{
    triggerStepMoveUp(pos + 1);
}

void RunStepListWidget::triggerRemoveRunStep(int pos)
{
    if (!m_runStepList->removeStep(pos)) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Removing Step failed"),
                             tr("Cannot remove build step while building"),
                             QMessageBox::Ok, QMessageBox::Ok);
    }
}

void RunStepListWidget::removeRunStep(int pos)
{
    delete m_runStepsData.takeAt(pos);

    updateRunStepButtonsState();

    bool hasSteps = m_runStepList->isEmpty();
    m_noStepsLabel->setVisible(hasSteps);
}

void RunStepListWidget::triggerDisable(int pos)
{
    RunStep *bs = m_runStepsData.at(pos)->step;
    bs->setEnabled(!bs->enabled());
    m_runStepsData.at(pos)->toolWidget->setRunStepEnabled(bs->enabled());
}

void RunStepListWidget::setupUi()
{
    if (0 != m_addButton)
        return;

    m_disableMapper = new QSignalMapper(this);
    connect(m_disableMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &RunStepListWidget::triggerDisable);
    m_upMapper = new QSignalMapper(this);
    connect(m_upMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &RunStepListWidget::triggerStepMoveUp);
    m_downMapper = new QSignalMapper(this);
    connect(m_downMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &RunStepListWidget::triggerStepMoveDown);
    m_removeMapper = new QSignalMapper(this);
    connect(m_removeMapper, static_cast<void (QSignalMapper::*)(int)>(&QSignalMapper::mapped),
            this, &RunStepListWidget::triggerRemoveRunStep);

    m_vbox = new QVBoxLayout(this);
    m_vbox->setContentsMargins(0, 0, 0, 0);
    m_vbox->setSpacing(0);

    m_noStepsLabel = new QLabel(tr("No Run Steps"), this);
    m_noStepsLabel->setContentsMargins(0, 0, 0, 0);
    m_vbox->addWidget(m_noStepsLabel);

    QHBoxLayout *hboxLayout = new QHBoxLayout();
    hboxLayout->setContentsMargins(0, 4, 0, 0);
    m_addButton = new QPushButton(this);
    m_addButton->setMenu(new QMenu(this));
    hboxLayout->addWidget(m_addButton);

    hboxLayout->addStretch(10);

    if (Utils::HostOsInfo::isMacHost())
        m_addButton->setAttribute(Qt::WA_MacSmallSize);

    m_vbox->addLayout(hboxLayout);

    connect(m_addButton->menu(), &QMenu::aboutToShow,
            this, &RunStepListWidget::updateAddRunStepMenu);
}

void RunStepListWidget::updateRunStepButtonsState()
{
    if (m_runStepsData.count() != m_runStepList->count())
        return;
    for (int i = 0; i < m_runStepsData.count(); ++i) {
        RunStepsWidgetData *s = m_runStepsData.at(i);
        m_disableMapper->setMapping(s->toolWidget, i);
        s->toolWidget->setRemoveEnabled(!m_runStepList->at(i)->immutable());
        m_removeMapper->setMapping(s->toolWidget, i);

        s->toolWidget->setUpEnabled((i > 0)
                                    && !(m_runStepList->at(i)->immutable()
                                         && m_runStepList->at(i - 1)->immutable()));
        m_upMapper->setMapping(s->toolWidget, i);
        s->toolWidget->setDownEnabled((i + 1 < m_runStepList->count())
                                      && !(m_runStepList->at(i)->immutable()
                                           && m_runStepList->at(i + 1)->immutable()));
        m_downMapper->setMapping(s->toolWidget, i);

        // Only show buttons when needed
        s->toolWidget->setDownVisible(m_runStepList->count() != 1);
        s->toolWidget->setUpVisible(m_runStepList->count() != 1);
    }
}

RunStepsPage::RunStepsPage(ROSRunConfiguration *rc) :
    NamedWidget(),
    m_widget(new RunStepListWidget(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_widget);

    m_widget->init(rc->stepList());
    setDisplayName(tr("ROS Run Steps"));
//    if (m_id == Constants::BUILDSTEPS_BUILD)
//        setDisplayName(tr("Build Steps"));
//    if (m_id == Constants::BUILDSTEPS_CLEAN)
//        setDisplayName(tr("Clean Steps"));
}

RunStepsPage::~RunStepsPage()
{ }

} // namespace Internal
} // namespace ROSProjectManager
