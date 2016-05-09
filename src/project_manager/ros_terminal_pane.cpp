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

#include <QDir>
#include <QSettings>


#include <signal.h>

namespace ROSProjectManager {
namespace Internal {

ROSTerminalPane::ROSTerminalPane() :
//  m_terminalWidget(new TerminalTabWidget),
  m_stopButton(new QToolButton),
  m_newTerminalButton(new QToolButton)

{
    m_stopButton->setToolTip(tr("Kill active terminal's child process"));
    m_stopButton->setIcon(Core::Icons::STOP_SMALL.icon());
    m_newTerminalButton->setToolTip(tr("Add new terminal"));
    m_newTerminalButton->setIcon(Core::Icons::NEWFILE.icon());

    connect(m_stopButton, &QToolButton::clicked,
            this, &ROSTerminalPane::stopProcess);

    connect(m_newTerminalButton, &QToolButton::clicked,
            this, &ROSTerminalPane::startTerminalButton);

    connect(m_terminalWidget, SIGNAL(terminalCreated(int)), this, SIGNAL(navigateStateUpdate));
    connect(m_terminalWidget, SIGNAL(terminalClosed(int)), this, SIGNAL(navigateStateUpdate));
}

ROSTerminalPane::~ROSTerminalPane()
{
  delete m_terminalWidget;
  delete m_stopButton;
  delete m_newTerminalButton;
}

void ROSTerminalPane::startTerminalButton()
{
  m_terminalWidget->createTerminal();
}

void ROSTerminalPane::stopProcess()
{
  m_terminalWidget->stopTerminalProcess();
//  foreach(QObject *obj, m_terminalWidget->ch->currentWidget()->children())
//  {
//    if(QLatin1String(obj->metaObject()->className()) == QLatin1String("Konsole::TerminalDisplay"))
//    {
//      QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
//      QCoreApplication::postEvent(obj, event1);

//      QKeyEvent *event2 = new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier);
//      QCoreApplication::postEvent(obj, event2);
//      break;
//    }
//  }
}

int ROSTerminalPane::createTerminal(const QString &name, const QDir &workingDirectory)
{
  int id = m_terminalWidget->createTerminal(name, workingDirectory);
  emit navigateStateUpdate();
  return id;
}

int ROSTerminalPane::createTerminal(const QDir &workingDirectory)
{
  int id = m_terminalWidget->createTerminal(workingDirectory);
  emit navigateStateUpdate();
  return id;
}

int ROSTerminalPane::createTerminal()
{
  int id = m_terminalWidget->createTerminal();
  emit navigateStateUpdate();
  return id;
}

QList<QWidget*> ROSTerminalPane::toolBarWidgets() const
{
  return QList<QWidget *>() << m_newTerminalButton
                            << m_stopButton;
}

bool ROSTerminalPane::hasFocus() const
{
    return m_terminalWidget->hasFocus();
}

bool ROSTerminalPane::canFocus() const
{
    return true;
}

void ROSTerminalPane::setFocus()
{
    m_terminalWidget->setFocus();
}

void ROSTerminalPane::clearContents()
{
//    qobject_cast<QTermWidget *>(m_tabWidget->currentWidget())->clear();
}

QWidget *ROSTerminalPane::outputWidget(QWidget *parent)
{
    m_terminalWidget->setParent(parent);
    return m_terminalWidget;
}

QString ROSTerminalPane::displayName() const
{
    return tr("ROS Terminals");
}

void ROSTerminalPane::visibilityChanged(bool /*b*/)
{
}

void ROSTerminalPane::sendInput(const QString &text)
{
  m_terminalWidget->sendInput(text);
}

void ROSTerminalPane::sendInput(const int &id, const QString &text)
{
  m_terminalWidget->sendInput(id, text);
}

int ROSTerminalPane::priorityInStatusBar() const
{
    return -1;
}

bool ROSTerminalPane::canNext() const
{
  return ((m_terminalWidget->count() > 1 && m_terminalWidget->currentIndex() < (m_terminalWidget->count()-1)) ? true : false);
}

bool ROSTerminalPane::canPrevious() const
{
  return (m_terminalWidget->count() > 1 && m_terminalWidget->currentIndex() > 0 ? true : false);
}

void ROSTerminalPane::goToNext()
{
  m_terminalWidget->setCurrentIndex(m_terminalWidget->currentIndex()+1);
  emit navigateStateChanged();
}

void ROSTerminalPane::goToPrev()
{
  m_terminalWidget->setCurrentIndex(m_terminalWidget->currentIndex()-1);
  emit navigateStateChanged();
}

bool ROSTerminalPane::canNavigate() const
{
    return true;
}

} // namespace Internal
} // namespace ROSProjectManager
