#ifndef ROSOUTPUTPANE_H
#define ROSOUTPUTPANE_H

#include <coreplugin/ioutputpane.h>

#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>

#include <QTabWidget>
#include <QToolButton>
#include <QProcess>

#include "qtermwidget.h"

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

  QTermWidget &startTerminal(int startnow = 1, const QString name = QLatin1String("Terminal"));

  void sendText(const QString &text);

private slots:
  void updateZoomEnabled();
  void zoomIn();
  void zoomOut();
  void startTerminalButton();
  void stopProcess();
  void closeTerminal(int index);

private:
  QList<QTermWidget *> m_terminals;
  QTabWidget * m_tabWidget;
  QStringList m_tabNames;

  QToolButton *m_stopButton;
  QToolButton *m_zoomInButton;
  QToolButton *m_zoomOutButton;
  QToolButton *m_newTerminalButton;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSOUTPUTPANE_H
