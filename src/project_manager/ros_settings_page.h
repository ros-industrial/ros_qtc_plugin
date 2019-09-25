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
#ifndef ROS_SETTINGS_PAGE_H
#define ROS_SETTINGS_PAGE_H

#include "ros_utils.h"

#include <coreplugin/dialogs/ioptionspage.h>

#include <QWidget>
#include <QPointer>
#include <QStringListModel>

namespace ROSProjectManager {
namespace Internal {

namespace Ui {
class ROSSettingsPage;
}

struct ROSSettings
{
    ROSSettings();

    QString default_distribution;

    ROSUtils::BuildSystem default_build_system;

    QString default_code_style;

    QString default_dist_path;

    QString custom_dist_path;

    void toSettings(QSettings *) const;
    void fromSettings(QSettings *);

    bool equals(const ROSSettings &rhs) const;
    bool operator==(const ROSSettings &s) const { return equals(s); }
    bool operator!=(const ROSSettings &s) const { return !equals(s); }

private:
    QList<Utils::FilePath> m_system_distributions;
};

class ROSSettingsWidget : public QWidget
{
  Q_OBJECT

public:
  explicit ROSSettingsWidget(QWidget *parent = 0);
  ~ROSSettingsWidget();

  ROSSettings settings() const;
  void setSettings(const ROSSettings &settings);

private:
  Ui::ROSSettingsPage *m_ui;
  QStringListModel *m_available_code_style_names;
  QMap<QString, QString> m_available_code_styles;
};

class ROSSettingsPage : public Core::IOptionsPage
{
public:
    explicit ROSSettingsPage(QSharedPointer<ROSSettings> &settings,
                             QObject *parent = nullptr);

    QWidget *widget();
    void apply();
    void finish();

private:
    const QSharedPointer<ROSSettings> m_settings;
    QPointer<ROSSettingsWidget> m_widget;
};

} // namespace Internal
} // namespace ROSProjectManager

#endif // ROS_SETTINGS_PAGE_H
