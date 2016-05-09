#include "terminal_tab_widget.h"
#include <kde_terminal_interface_v2.h>
#include <QMessageBox>
#include <QDebug>
#include <QDir>
#include <QKeySequence>
#include <QKeyEvent>
#include <QApplication>

TerminalTabWidget::TerminalTabWidget(QWidget *parent) :
  m_tabWidget(new QTabWidget(parent)),
  m_counter(0)
{
  m_tabWidget->setTabsClosable(true);

  m_service = KService::serviceByDesktopName(QLatin1Literal("konsolepart"));
  if (!m_service)
  {
    QMessageBox::critical(this, tr("Konsole not installed"), tr("Please install the kde konsole and try again!"), QMessageBox::Ok);
    return;
  }

  connect(m_tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTerminal(int)));
}

TerminalTabWidget::~TerminalTabWidget()
{
  delete m_tabWidget;
  foreach (KParts::ReadOnlyPart* part, m_terminals)
  {
    delete part;
  }
}

int TerminalTabWidget::createTerminal()
{
  return createTerminal(QString::fromLatin1("Terminal%1").arg(m_counter+1));
}

int TerminalTabWidget::createTerminal(const QDir &workingDirectory)
{
  return createTerminal(QString::fromLatin1("Terminal%1").arg(m_counter+1), workingDirectory);
}

int TerminalTabWidget::createTerminal(const QString &name, const QDir &workingDirectory)
{

  // create one instance of konsolepart
  KParts::ReadOnlyPart *part = m_service->createInstance<KParts::ReadOnlyPart>(this, this, QVariantList());
  if (!part)
  {
    qDebug() << "Failed to create a new terminal.";
    return -1;
  }

  if (workingDirectory.exists() && !(workingDirectory.absolutePath() == workingDirectory.homePath()))
    qobject_cast<TerminalInterface*>(part)->showShellInDir(workingDirectory.absolutePath());

  int id = m_tabWidget->addTab(part->widget(), name);
  m_tabWidget->setCurrentIndex(id);

  m_terminals[id] = part;

  emit terminalCreated(id);
  return id;
}

void TerminalTabWidget::closeTerminal(const int &id)
{
  delete m_terminals[id];
  m_tabWidget->removeTab(id);
  m_terminals.remove(id);
  emit terminalClosed(id);
}

void TerminalTabWidget::sendInput(const int &id, const QString &text)
{
  qobject_cast<TerminalInterfaceV2*>(m_terminals[id])->sendInput(text);
}

void TerminalTabWidget::sendInput(const QString &text)
{
  qobject_cast<TerminalInterfaceV2*>(m_terminals[m_tabWidget->currentIndex()])->sendInput(text);
}

void TerminalTabWidget::stopTerminalProcess()
{
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
  stopTerminalProcess(m_tabWidget->currentIndex());

}

void TerminalTabWidget::stopTerminalProcess(int id)
{
  QKeyEvent *event1 = new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
  QCoreApplication::postEvent(m_terminals[id]->widget(), event1);

  QKeyEvent *event2 = new QKeyEvent(QEvent::KeyRelease, Qt::Key_C, Qt::ControlModifier);
  QCoreApplication::postEvent(m_terminals[id]->widget(), event2);
}

int TerminalTabWidget::currentTerminalId() const
{
  return m_tabWidget->currentIndex();
}

void TerminalTabWidget::setCurrentTerminal(int id)
{
  m_tabWidget->setCurrentIndex(id);
}

int TerminalTabWidget::currentIndex() const
{
  return m_tabWidget->currentIndex();
}

void TerminalTabWidget::setCurrentIndex(int index)
{
  m_tabWidget->setCurrentIndex(index);
}

int TerminalTabWidget::count() const
{
  return m_tabWidget->count();
}
