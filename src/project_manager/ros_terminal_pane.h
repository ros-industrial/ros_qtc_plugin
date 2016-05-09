#ifndef ROSOUTPUTPANE_H
#define ROSOUTPUTPANE_H

#include "terminal_tab_widget.h"

#include <coreplugin/ioutputpane.h>
#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>

#include <QTabWidget>
#include <QToolButton>
#include <QProcess>

namespace ROSProjectManager {
namespace Internal {


class ROSTerminalPane : public Core::IOutputPane
{
  Q_OBJECT

public:
  ROSTerminalPane();
  ~ROSTerminalPane();

  QWidget *outputWidget(QWidget *parent) override;
  QList<QWidget *> toolBarWidgets() const override;
  QString displayName() const override;

  int priorityInStatusBar() const override;

  void clearContents() override;
  void visibilityChanged(bool visible) override;

  void setFocus() override;
  bool hasFocus() const override;
  bool canFocus() const override;

  bool canNavigate() const override;
  bool canNext() const override;
  bool canPrevious() const override;
  void goToNext() override;
  void goToPrev() override;

  int createTerminal(const QString &name, const QDir &workingDirectory = QDir());
  int createTerminal(const QDir &workingDirectory);
  int createTerminal();

  void sendInput(const QString &text);
  void sendInput(const int &id, const QString &text);

private slots:
  void startTerminalButton();
  void stopProcess();

private:

  TerminalTabWidget *m_terminalWidget;
  QToolButton *m_stopButton;
  QToolButton *m_newTerminalButton;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSOUTPUTPANE_H
