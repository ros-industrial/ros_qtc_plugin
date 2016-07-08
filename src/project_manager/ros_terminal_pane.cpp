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
#include "ros_terminal_pane.h"
#include "ros_project.h"
#include "ros_build_configuration.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/projectexplorericons.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitmanager.h>

#include <coreplugin/icontext.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/find/basetextfind.h>
#include <coreplugin/coreicons.h>
#include <coreplugin/icore.h>

#include <aggregation/aggregate.h>
#include <cstring>

#include <texteditor/texteditorsettings.h>
#include <texteditor/fontsettings.h>
#include <texteditor/behaviorsettings.h>

#include <QDir>
#include <QSettings>
#include <QKeySequence>
#include <QKeyEvent>
#include <QApplication>

#include <signal.h>

namespace ROSProjectManager {
namespace Internal {

ROSTerminalPane::ROSTerminalPane() :
  m_stopButton(new QToolButton),
  m_zoomInButton(new QToolButton),
  m_zoomOutButton(new QToolButton),
  m_newTerminalButton(new QToolButton)
{
    m_tabWidget = new QTabWidget();
    m_tabWidget->setTabsClosable(true);

    m_stopButton->setToolTip(tr("Kill active terminal's child process"));
    m_stopButton->setIcon(Core::Icons::STOP_SMALL.icon());
    m_zoomInButton->setToolTip(tr("Increase Font Size"));
    m_zoomInButton->setIcon(Core::Icons::PLUS.icon());
    m_zoomOutButton->setToolTip(tr("Decrease Font Size"));
    m_zoomOutButton->setIcon(Core::Icons::MINUS.icon());
    m_newTerminalButton->setToolTip(tr("Add new terminal"));
    m_newTerminalButton->setIcon(Core::Icons::NEWFILE.icon());

    updateToolBarButtonsEnabled();

    connect(TextEditor::TextEditorSettings::instance(),
                &TextEditor::TextEditorSettings::behaviorSettingsChanged,
                this, &ROSTerminalPane::updateToolBarButtonsEnabled);

    connect(m_zoomInButton, &QToolButton::clicked,
                this, &ROSTerminalPane::zoomIn);

    connect(m_zoomOutButton, &QToolButton::clicked,
            this, &ROSTerminalPane::zoomOut);

    connect(m_stopButton, &QToolButton::clicked,
            this, &ROSTerminalPane::stopProcess);

    connect(m_newTerminalButton, &QToolButton::clicked,
            this, &ROSTerminalPane::startTerminalButton);

    connect(m_tabWidget,SIGNAL(tabCloseRequested(int)), this, SLOT(closeTerminal(int)));
}

ROSTerminalPane::~ROSTerminalPane()
{
  delete m_tabWidget;
  delete m_stopButton;
  delete m_zoomInButton;
  delete m_zoomOutButton;
  delete m_newTerminalButton;
}

void ROSTerminalPane::updateToolBarButtonsEnabled()
{
  const TextEditor::BehaviorSettings &settings
          = TextEditor::TextEditorSettings::behaviorSettings();

  bool hasTerminal = (m_terminals.count() > 0);
  bool zoomEnabled  = settings.m_scrollWheelZooming && hasTerminal;

  m_stopButton->setEnabled(hasTerminal);
  m_zoomInButton->setEnabled(zoomEnabled);
  m_zoomOutButton->setEnabled(zoomEnabled);
}

void ROSTerminalPane::zoomIn()
{
  foreach(QTermWidget *ow, m_terminals)
  {
    ow->zoomIn();
  }
}

void ROSTerminalPane::zoomOut()
{
  foreach(QTermWidget *ow, m_terminals)
  {
    ow->zoomOut();
  }
}

void ROSTerminalPane::startTerminalButton()
{
  startTerminal();
}

void ROSTerminalPane::closeTerminal(int index)
{
  m_tabNames.removeAll(m_tabWidget->tabText(index));
  m_terminals.removeAll(qobject_cast<QTermWidget*>(m_tabWidget->currentWidget()));
  m_tabWidget->removeTab(index);

  updateToolBarButtonsEnabled();

  emit navigateStateUpdate();
}

void ROSTerminalPane::termKeyPressed(QKeyEvent *event)
{
    if (event->modifiers().testFlag(Qt::ControlModifier) && event->modifiers().testFlag(Qt::ShiftModifier))
    {
        if (event->key() == Qt::Key_C)
        {
            qobject_cast<QTermWidget *>(m_tabWidget->currentWidget())->copyClipboard();
        }
        else if (event->key() == Qt::Key_V)
        {
            qobject_cast<QTermWidget *>(m_tabWidget->currentWidget())->pasteClipboard();
        }
    }
}

void ROSTerminalPane::stopProcess()
{
  foreach(QObject *obj, m_tabWidget->currentWidget()->children())
  {
    if(QLatin1String(obj->metaObject()->className()) == QLatin1String("Konsole::TerminalDisplay"))
    {
      QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
      QCoreApplication::postEvent(obj, event1);

      QKeyEvent *event2 = new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier);
      QCoreApplication::postEvent(obj, event2);
      break;
    }
  }
}

QTermWidget &ROSTerminalPane::startTerminal(int startnow, const QString name)
{
  QString tabName = name;
  QString tabNamePrefix;

//  const char *index = std::to_string(m_terminals.count()).c_str();
//  char *paneId = new char[std::strlen(ROS_OUTPUT_WINDOW_PREFIX)+std::strlen(index)+1];
//  std::strcpy(paneId, ROS_OUTPUT_WINDOW_PREFIX);
//  std::strcat(paneId, index);

  //don't start shell yet
  QTermWidget *widget = new QTermWidget(startnow);
  widget->setScrollBarPosition(QTermWidget::ScrollBarRight);

  // This is to capture copy and paste key events
  connect(widget, SIGNAL(termKeyPressed(QKeyEvent*)), this, SLOT(termKeyPressed(QKeyEvent*)));

  QSettings *s = Core::ICore::settings();
  s->beginGroup(QLatin1String("ROSTerminal"));

  // Need to create a qtc dark color scheme for the terminal
  // Example: https://github.com/lxde/qtermwidget/blob/10e17968e4457da2b91675984e17009ee6e1e7aa/lib/color-schemes/Linux.colorscheme
  widget->setColorScheme(s->value(QLatin1String("ColorScheme"), QLatin1String("DarkPastels")).toString());
//  QFont f(s->value(QLatin1String("FontName"), TERMINALPLUGINDEFAULTFONT).toString());
//  f.setPointSize(s->value(QLatin1String("FontSize"), 10).toInt());
//  widget->setTerminalFont(f);
  s->endGroup();

  m_terminals.append(widget);

  updateToolBarButtonsEnabled();

  //Create a unique tab name
  tabNamePrefix = tabName;
  int cnt=0;
  while(m_tabNames.contains(tabName))
  {
    tabName = QString::fromLatin1("%1(%2)").arg(tabNamePrefix, QString::number(cnt));
    cnt+=1;
  };

  m_tabNames.append(tabName);
  int idx = m_tabWidget->addTab(widget, tabName);
  m_tabWidget->setCurrentIndex(idx);

  emit navigateStateUpdate();
  return *widget;
}

QList<QWidget*> ROSTerminalPane::toolBarWidgets() const
{
  return QList<QWidget *>() << m_newTerminalButton
                            << m_stopButton
                            << m_zoomInButton
                            << m_zoomOutButton;
}

bool ROSTerminalPane::hasFocus() const
{
    return m_tabWidget->window()->focusWidget() == m_tabWidget;
}

bool ROSTerminalPane::canFocus() const
{
    return true;
}

void ROSTerminalPane::setFocus()
{
    m_tabWidget->setFocus();
}

void ROSTerminalPane::clearContents()
{
    if (m_terminals.count() > 0)
        qobject_cast<QTermWidget *>(m_tabWidget->currentWidget())->clear();
}

QWidget *ROSTerminalPane::outputWidget(QWidget *parent)
{
    m_tabWidget->setParent(parent);
    return m_tabWidget;
}

QString ROSTerminalPane::displayName() const
{
    return tr("ROS Terminals");
}

void ROSTerminalPane::visibilityChanged(bool /*b*/)
{
}

void ROSTerminalPane::sendText(const QString &text)
{
    qobject_cast<QTermWidget *>(m_tabWidget->currentWidget())->sendText(text);
}

int ROSTerminalPane::priorityInStatusBar() const
{
    return -1;
}

bool ROSTerminalPane::canNext() const
{
  if(m_terminals.count() > 1 && m_tabWidget->currentIndex() < (m_terminals.count()-1))
  {
    return true;
  }

  return false;
}

bool ROSTerminalPane::canPrevious() const
{
  if(m_terminals.count() > 1 && m_tabWidget->currentIndex() > 0)
  {
    return true;
  }

  return false;
}

void ROSTerminalPane::goToNext()
{
  m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()+1);
  emit navigateStateChanged();
}

void ROSTerminalPane::goToPrev()
{
  m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()-1);
  emit navigateStateChanged();
}

bool ROSTerminalPane::canNavigate() const
{
    return true;
}

} // namespace Internal
} // namespace ROSProjectManager
