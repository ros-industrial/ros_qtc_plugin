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

ToolWidget::ToolWidget(QWidget *parent) : FadingPanel(parent)
{
    auto layout = new QHBoxLayout;
    layout->setMargin(4);
    layout->setSpacing(4);
    setLayout(layout);
    m_firstWidget = new Utils::FadingWidget(this);
    m_firstWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    auto hbox = new QHBoxLayout();
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
            m_firstWidget->setOpacity(.999);
        else
            m_firstWidget->fadeTo(.999);
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
    step(s), widget(nullptr), detailsWidget(nullptr)
{
    widget = s->createConfigWidget();
    Q_ASSERT(widget);

    detailsWidget = new Utils::DetailsWidget();
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
    NamedWidget(tr("Steps"), parent)
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
        for (const RunStepsWidgetData *s : m_runStepsData) {
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
        for (const RunStepsWidgetData *s : m_runStepsData) {
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
        for (const RunStepsWidgetData *s : m_runStepsData) {
            if (s->step == step) {
                s->toolWidget->setRunStepEnabled(step->enabled());
                break;
            }
        }
    }
    updateRunStepButtonsState();
}

void RunStepListWidget::init(RunStepList *rsl)
{
    Q_ASSERT(rsl);
    if (rsl == m_runStepList)
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

    connect(rsl, &RunStepList::stepInserted, this, &RunStepListWidget::addRunStep);
    connect(rsl, &RunStepList::stepRemoved, this, &RunStepListWidget::removeRunStep);
    connect(rsl, &RunStepList::stepMoved, this, &RunStepListWidget::stepMoved);

    qDeleteAll(m_runStepsData);
    m_runStepsData.clear();

    m_runStepList = rsl;

    for (int i = 0; i < rsl->count(); ++i) {
        addRunStep(i);
        // addBuilStep expands the config widget by default, which we don't want here
        if (m_runStepsData.at(i)->widget->showWidget())
            m_runStepsData.at(i)->detailsWidget->setState(Utils::DetailsWidget::Collapsed);
    }

    m_noStepsLabel->setVisible(rsl->isEmpty());
    m_noStepsLabel->setText(tr("No %1 Steps").arg(m_runStepList->displayName()));

    m_addButton->setText(tr("Add %1 Step").arg(m_runStepList->displayName()));

    updateRunStepButtonsState();
}

void RunStepListWidget::updateAddRunStepMenu()
{
  QMap<QString, QPair<Core::Id, RunStepFactory *> > map;
  //Build up a list of possible steps and save map the display names to the (internal) name and factories.
  for (RunStepFactory *factory : RunStepFactory::allRunStepFactories()) {
      if (factory->canHandle(m_runStepList)) {
          const RunStepInfo &info = factory->stepInfo();
          if (info.flags & RunStepInfo::Uncreatable)
              continue;
          if ((info.flags & RunStepInfo::UniqueStep) && m_runStepList->contains(info.id))
              continue;
          map.insert(info.displayName, qMakePair(info.id, factory));
      }
  }

  // Ask the user which one to add
  QMenu *menu = m_addButton->menu();
  menu->clear();
  if (!map.isEmpty()) {
      QMap<QString, QPair<Core::Id, RunStepFactory *> >::const_iterator it, end;
      end = map.constEnd();
      for (it = map.constBegin(); it != end; ++it) {
          QAction *action = menu->addAction(it.key());
          RunStepFactory *factory = it.value().second;
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

void RunStepListWidget::stepMoved(int from, int to)
{
    m_vbox->insertWidget(to, m_runStepsData.at(from)->detailsWidget);

    RunStepsWidgetData *data = m_runStepsData.at(from);
    m_runStepsData.removeAt(from);
    m_runStepsData.insert(to, data);

    updateRunStepButtonsState();
}

void RunStepListWidget::removeRunStep(int pos)
{
    delete m_runStepsData.takeAt(pos);

    updateRunStepButtonsState();

    bool hasSteps = m_runStepList->isEmpty();
    m_noStepsLabel->setVisible(hasSteps);
}

void RunStepListWidget::setupUi()
{
    if (m_addButton)
        return;

    m_vbox = new QVBoxLayout(this);
    m_vbox->setContentsMargins(0, 0, 0, 0);
    m_vbox->setSpacing(0);

    m_noStepsLabel = new QLabel(tr("No Run Steps"), this);
    m_noStepsLabel->setContentsMargins(0, 0, 0, 0);
    m_vbox->addWidget(m_noStepsLabel);

    auto hboxLayout = new QHBoxLayout();
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
        disconnect(s->toolWidget, nullptr, this, nullptr);
        connect(s->toolWidget, &ToolWidget::disabledClicked,
                this, [s] {
            RunStep *rs = s->step;
            rs->setEnabled(!rs->enabled());
            s->toolWidget->setRunStepEnabled(rs->enabled());
        });
        s->toolWidget->setRemoveEnabled(!m_runStepList->at(i)->immutable());
        connect(s->toolWidget, &ToolWidget::removeClicked,
                this, [this, i] {
            if (!m_runStepList->removeStep(i)) {
                QMessageBox::warning(Core::ICore::mainWindow(),
                                     tr("Removing Step failed"),
                                     tr("Cannot remove run step"),
                                     QMessageBox::Ok, QMessageBox::Ok);
            }
        });

        s->toolWidget->setUpEnabled((i > 0)
                                    && !(m_runStepList->at(i)->immutable()
                                         && m_runStepList->at(i - 1)->immutable()));
        connect(s->toolWidget, &ToolWidget::upClicked,
                this, [this, i] { m_runStepList->moveStepUp(i); });
        s->toolWidget->setDownEnabled((i + 1 < m_runStepList->count())
                                      && !(m_runStepList->at(i)->immutable()
                                           && m_runStepList->at(i + 1)->immutable()));
        connect(s->toolWidget, &ToolWidget::downClicked,
                this, [this, i] { m_runStepList->moveStepUp(i + 1); });

        // Only show buttons when needed
        s->toolWidget->setDownVisible(m_runStepList->count() != 1);
        s->toolWidget->setUpVisible(m_runStepList->count() != 1);
    }
}

RunStepsPage::RunStepsPage(ROSRunConfiguration *rc, Core::Id id) :
    NamedWidget(tr("ROS Run Steps")),
    m_id(id),
    m_widget(new RunStepListWidget(this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_widget);

    m_widget->init(rc->stepList());
}

} // namespace Internal
} // namespace ROSProjectManager
