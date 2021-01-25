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
#ifndef ROSOUTPUTPANE_H
#define ROSOUTPUTPANE_H

#include <coreplugin/ioutputpane.h>

#include <projectexplorer/processparameters.h>
#include <utils/qtcprocess.h>

#include <QProcess>
#include <QTabWidget>
#include <QToolButton>

#include <qtermwidget5/qtermwidget.h>

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
    void updateToolBarButtonsEnabled();
    void zoomIn();
    void zoomOut();
    void startTerminalButton();
    void stopProcess();
    void closeTerminal(int index);
    void termKeyPressed(QKeyEvent *event);

private:
    QList<QTermWidget *> m_terminals;
    QTabWidget *m_tabWidget;
    QStringList m_tabNames;

    QToolButton *m_stopButton;
    QToolButton *m_zoomInButton;
    QToolButton *m_zoomOutButton;
    QToolButton *m_newTerminalButton;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROSOUTPUTPANE_H
