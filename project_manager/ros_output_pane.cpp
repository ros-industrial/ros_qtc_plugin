#include "ros_output_pane.h"

#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/projectexplorericons.h>

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

namespace ROSProjectManager {
namespace Internal {
const char ROS_OUTPUT_WINDOW_PREFIX[] = "ROSProjectManager.ROSOutputPanes.Window";
const char SETTINGS_KEY[] = "ROSProject/OutputWindow/Zoom";
const int MAX_LINECOUNT = 100000;

ROSOutputWindow::ROSOutputWindow(Core::Context context, const ProjectExplorer::ProcessParameters &param) :
  Core::OutputWindow(context),
  m_processParameters(param)
{
  m_rosProcess = new Utils::QtcProcess();
  connect(m_rosProcess, &QProcess::readyReadStandardOutput,this, &ROSOutputWindow::processReadyReadStdOutput);
  connect(m_rosProcess, &QProcess::readyReadStandardError,this, &ROSOutputWindow::processReadyReadStdError);
  connect(m_rosProcess, static_cast<void (QProcess::*)(int,QProcess::ExitStatus)>(&QProcess::finished),
              this, &ROSOutputWindow::slotProcessFinished);
}

ROSOutputWindow::~ROSOutputWindow()
{
  delete m_rosProcess;
}

void ROSOutputWindow::startProcess()
{
  m_processParameters.resolveAll();
  m_rosProcess->setWorkingDirectory(m_processParameters.workingDirectory());
  m_rosProcess->setCommand(m_processParameters.effectiveCommand(),m_processParameters.effectiveArguments());
  m_rosProcess->start();
  m_rosProcess->waitForStarted();
}

void ROSOutputWindow::stopProcess()
{
  if(m_rosProcess)
  {
    m_rosProcess->terminate();
  }
}

void ROSOutputWindow::cleanUp()
{
  processFinished(m_rosProcess->exitCode(), m_rosProcess->exitStatus());
  delete m_rosProcess;
  m_rosProcess = 0;
  //emit finished();
}

void ROSOutputWindow::processFinished(int exitCode, QProcess::ExitStatus status)
{
  if (status == QProcess::NormalExit && exitCode == 0)
  {
    stdOutput(tr("The process exited normally."));
  }
  else if (status == QProcess::NormalExit)
  {
    stdOutput(tr("The process exited with code %1.").arg(QString::number(m_rosProcess->exitCode())));
  }
  else if (status == QProcess::CrashExit && exitCode == 15)
  {
    stdError(tr("The process terminated by user."));
  }
  else
  {
    stdError(tr("The process crashed."));
  }
}

void ROSOutputWindow::stdOutput(const QString &line)
{
  Utils::Theme *theme = Utils::creatorTheme();
  QTextCharFormat textFormat;
  textFormat.setForeground(theme->color(Utils::Theme::TextColorNormal));
  textFormat.setFontWeight(QFont::Normal);
  appendText(line + QLatin1String("\n"), textFormat);
  QCoreApplication::processEvents();
}

void ROSOutputWindow::processReadyReadStdOutput()
{
  QByteArray strdata = m_rosProcess->readAllStandardOutput();
  if(!strdata.isEmpty())
  {
    stdOutput(QString::fromLatin1(strdata.data()).simplified());
  }
}

void ROSOutputWindow::stdError(const QString &line)
{
  Utils::Theme *theme = Utils::creatorTheme();
  QTextCharFormat textFormat;
  textFormat.setForeground(theme->color(Utils::Theme::OutputPanes_ErrorMessageTextColor));
  textFormat.setFontWeight(QFont::Normal);
  appendText(line + QLatin1String("\n"), textFormat);
  QCoreApplication::processEvents();
}

void ROSOutputWindow::processReadyReadStdError()
{
  QByteArray strdata = m_rosProcess->readAllStandardError();
  stdError(QString::fromLatin1(strdata.data()).simplified());
}

void ROSOutputWindow::slotProcessFinished(int, QProcess::ExitStatus)
{
  processReadyReadStdError();
  processReadyReadStdOutput();
  cleanUp();
}

ROSOutputPane::ROSOutputPane() :
  m_stopButton(new QToolButton),
  m_zoomInButton(new QToolButton),
  m_zoomOutButton(new QToolButton)
{
    m_tabWidget = new QTabWidget();

    m_stopButton->setToolTip(tr("Kill Process"));
    m_stopButton->setIcon(ProjectExplorer::Icons::STOP_SMALL.icon());
    m_zoomInButton->setToolTip(tr("Increase Font Size"));
    m_zoomInButton->setIcon(Core::Icons::PLUS.icon());
    m_zoomOutButton->setToolTip(tr("Decrease Font Size"));
    m_zoomOutButton->setIcon(Core::Icons::MINUS.icon());

    connect(TextEditor::TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
                    this, SLOT(fontSettingsChanged()));

    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &ROSOutputPane::saveSettings);

    connect(TextEditor::TextEditorSettings::instance(),
                &TextEditor::TextEditorSettings::behaviorSettingsChanged,
                this, &ROSOutputPane::updateZoomEnabled);

    connect(ProjectExplorer::ProjectExplorerPlugin::instance(), SIGNAL(settingsChanged()),
                this, SLOT(updateWordWrapMode()));

    connect(m_zoomInButton, &QToolButton::clicked,
                this, &ROSOutputPane::zoomIn);

    connect(m_zoomOutButton, &QToolButton::clicked,
            this, &ROSOutputPane::zoomOut);

    connect(m_stopButton, &QToolButton::clicked,
            this, &ROSOutputPane::stopProcess);
}

ROSOutputPane::~ROSOutputPane()
{
  delete m_tabWidget;
  delete m_stopButton;
  delete m_zoomInButton;
  delete m_zoomOutButton;
}

void ROSOutputPane::fontSettingsChanged()
{
  foreach(ROSOutputWindow *ow, m_windows)
  {
    ow->setBaseFont(TextEditor::TextEditorSettings::fontSettings().font());
  }
}

void ROSOutputPane::saveSettings()
{
//  QSettings *settings = Core::ICore::settings();
//          settings->setValue(QLatin1String(SETTINGS_KEY), fontZoom());
}

void ROSOutputPane::updateZoomEnabled()
{
  const TextEditor::BehaviorSettings &settings
          = TextEditor::TextEditorSettings::behaviorSettings();
  bool zoomEnabled  = settings.m_scrollWheelZooming;
  m_zoomInButton->setEnabled(zoomEnabled);
  m_zoomOutButton->setEnabled(zoomEnabled);
  foreach(ROSOutputWindow *ow, m_windows)
  {
    ow->setWheelZoomEnabled(zoomEnabled);
  }
}

void ROSOutputPane::updateWordWrapMode()
{
  foreach(ROSOutputWindow *ow, m_windows)
  {
    ow->setWordWrapEnabled(ProjectExplorer::ProjectExplorerPlugin::projectExplorerSettings().wrapAppOutput);
  }
}

void ROSOutputPane::zoomIn()
{
  foreach(ROSOutputWindow *ow, m_windows)
  {
    ow->zoomIn(1);
  }
}

void ROSOutputPane::zoomOut()
{
  foreach(ROSOutputWindow *ow, m_windows)
  {
    ow->zoomOut(1);
  }
}

void ROSOutputPane::stopProcess()
{
  qobject_cast<ROSOutputWindow *>(m_tabWidget->currentWidget())->stopProcess();
}

void ROSOutputPane::startProcess(const ProjectExplorer::ProcessParameters &param)
{
  const char *index = std::to_string(m_windows.count()).c_str();
  char *paneId = new char[std::strlen(ROS_OUTPUT_WINDOW_PREFIX)+std::strlen(index)+1];
  std::strcpy(paneId, ROS_OUTPUT_WINDOW_PREFIX);
  std::strcat(paneId, index);

  ROSOutputWindow *widget = new ROSOutputWindow(Core::Context(paneId), param);
  widget->setReadOnly(true);
  widget->setWheelZoomEnabled(true);

  QSettings *settings = Core::ICore::settings();
  float zoom = settings->value(QLatin1String(SETTINGS_KEY), 0).toFloat();
  widget->setFontZoom(zoom);

  // Let selected text be colored as if the text edit was editable,
  // otherwise the highlight for searching is too light
  QPalette p = widget->palette();
  QColor activeHighlight = p.color(QPalette::Active, QPalette::Highlight);
  p.setColor(QPalette::Highlight, activeHighlight);
  QColor activeHighlightedText = p.color(QPalette::Active, QPalette::HighlightedText);
  p.setColor(QPalette::HighlightedText, activeHighlightedText);
  widget->setPalette(p);

  Aggregation::Aggregate *agg = new Aggregation::Aggregate;
  agg->add(widget);
  agg->add(new Core::BaseTextFind(widget));

  m_windows.append(widget);

  updateZoomEnabled();
  updateWordWrapMode();

  m_tabWidget->addTab(widget,tr("test"));
  widget->startProcess();
}

QList<QWidget*> ROSOutputPane::toolBarWidgets() const
{
  return QList<QWidget *>() << m_stopButton
                            << m_zoomInButton
                            << m_zoomOutButton;
}

bool ROSOutputPane::hasFocus() const
{
    return m_tabWidget->window()->focusWidget() == m_tabWidget;
}

bool ROSOutputPane::canFocus() const
{
    return true;
}

void ROSOutputPane::setFocus()
{
    m_tabWidget->setFocus();
}

void ROSOutputPane::clearContents()
{
    qobject_cast<ROSOutputWindow *>(m_tabWidget->currentWidget())->clear();
}

QWidget *ROSOutputPane::outputWidget(QWidget *parent)
{
    m_tabWidget->setParent(parent);
    return m_tabWidget;
}

QString ROSOutputPane::displayName() const
{
    return tr("ROS Terminals");
}

void ROSOutputPane::visibilityChanged(bool /*b*/)
{
}

//void ROSOutputPane::append(const QString &text)
//{
//    qobject_cast<ROSOutputWindow *>(m_tabWidget->currentWidget())->appendText(text);
//}

int ROSOutputPane::priorityInStatusBar() const
{
    return -1;
}

bool ROSOutputPane::canNext() const
{
  if(m_windows.count() > 1 && m_tabWidget->currentIndex() < m_windows.count())
  {
    return true;
  }

  return false;
}

bool ROSOutputPane::canPrevious() const
{
  if(m_windows.count() > 1 && m_tabWidget->currentIndex() > 0)
  {
    return true;
  }

  return false;
}

void ROSOutputPane::goToNext()
{
  m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()+1);
}

void ROSOutputPane::goToPrev()
{
  m_tabWidget->setCurrentIndex(m_tabWidget->currentIndex()-1);
}

bool ROSOutputPane::canNavigate() const
{
    return false;
}

} // namespace Internal
} // namespace ROSProjectManager
