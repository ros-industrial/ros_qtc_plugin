#ifndef ROSOUTPUTPANE_H
#define ROSOUTPUTPANE_H

#include <coreplugin/ioutputpane.h>
#include <coreplugin/outputwindow.h>

#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>

#include <QTabWidget>
#include <QToolButton>
#include <QProcess>

namespace ROSProjectManager {
namespace Internal {

class ROSOutputWindow :public Core::OutputWindow
{
  Q_OBJECT

public:
  ROSOutputWindow(Core::Context context, const ProjectExplorer::ProcessParameters &param);
  ~ROSOutputWindow();

  void startProcess();
  void stopProcess();

protected:
  void processFinished(int exitCode, QProcess::ExitStatus status);
  void stdOutput(const QString &line);
  void stdError(const QString &line);

private:
  void cleanUp();
  void processReadyReadStdError();
  void processReadyReadStdOutput();
  void slotProcessFinished(int, QProcess::ExitStatus);

  ProjectExplorer::ProcessParameters m_processParameters;
  Utils::QtcProcess *m_rosProcess;
};

class ROSOutputPane : public Core::IOutputPane
{
  Q_OBJECT

public:
  ROSOutputPane();
  ~ROSOutputPane();

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

  void startProcess(const ProjectExplorer::ProcessParameters &param);
  void saveSettings();

  //void appendText(const QString &text, BuildStep::OutputFormat format);
private slots:
  void fontSettingsChanged();
  void updateZoomEnabled();
  void updateWordWrapMode();
  void zoomIn();
  void zoomOut();
  void stopProcess();

private:
  QList<ROSOutputWindow *> m_windows;
  QTabWidget * m_tabWidget;

  QToolButton *m_stopButton;
  QToolButton *m_zoomInButton;
  QToolButton *m_zoomOutButton;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSOUTPUTPANE_H
