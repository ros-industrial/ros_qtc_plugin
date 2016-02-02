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

#ifndef ROS_RUN_CONFIGURATION_H
#define ROS_RUN_CONFIGURATION_H

#include <projectexplorer/localapplicationrunconfiguration.h>

#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QStringListModel)


namespace ROSProjectManager {
class ROSProject;

namespace Internal {

class ROSRunConfigurationFactory;
class ROSRunConfigurationWidget;

class ROSRunConfiguration : public ProjectExplorer::LocalApplicationRunConfiguration
{
    Q_OBJECT
    friend class ROSRunConfigurationFactory;
    friend class ROSRunConfigurationWidget;
    friend class ROSProject; // to call updateEnabled()

public:
    ROSRunConfiguration(ProjectExplorer::Target *parent, Core::Id id);

    QString executable() const override;
    ProjectExplorer::ApplicationLauncher::Mode runMode() const override;
    QString commandLineArguments() const override;

    QString workingDirectory() const override;

    // RunConfiguration
    bool isEnabled() const override;
    QString disabledReason() const override;
    virtual QWidget *createConfigurationWidget() override;
    Utils::OutputFormatter *createOutputFormatter() const override;
    QVariantMap toMap() const override;

    ProjectExplorer::Abi abi() const override;

protected:
    ROSRunConfiguration(ProjectExplorer::Target *parent,
                               QmlProjectRunConfiguration *source);
    virtual bool fromMap(const QVariantMap &map) override;
//    void setEnabled(bool value);

private:
    void ctor();
    static bool isValidVersion(QtSupport::BaseQtVersion *version);

    static QString canonicalCapsPath(const QString &filePath);

    // absolute path to current file (if being used)
    QString m_currentFileFilename;
    // absolute path to selected main script (if being used)
    QString m_mainScriptFilename;

    QString m_scriptFile;
    QString m_qmlViewerArgs;

    bool m_isEnabled;
};
}

} // namespace ROSProjectManager

#endif // ROS_RUN_CONFIGURATION_H
