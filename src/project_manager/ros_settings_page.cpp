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
#include "ros_settings_page.h"
#include "ros_project_constants.h"
#include "ui_ros_settings_page.h"
#include "ros_project_constants.h"
#include <QDir>

#include <coreplugin/icore.h>

#include <texteditor/icodestylepreferences.h>
#include <texteditor/icodestylepreferencesfactory.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/codestylepool.h>

#include <cpptools/cpptoolsconstants.h>

#include <projectexplorer/projectexplorerconstants.h>

static const char DEFAULT_DISTRIBUTION_ID[] = "ROSProjectManager.ROSSettingsDefaultDistribution";
static const char DEFAULT_BUILD_SYSTEM_ID[] = "ROSProjectManager.ROSSettingsDefaultBuildSystem";
static const char DEFAULT_CODE_STYLE_ID[] = "ROSProjectManager.ROSSettingsDefaultCodeStyle";
static const char DEFAULT_DISTRIBUTION_PATH_ID[] = "ROSProjectManager.ROSSettingsDefaultDistributionPath";
static const char CUSTOM_DISTRIBUTION_PATH_ID[] = "ROSProjectManager.ROSSettingsCustomDistributionPath";

namespace ROSProjectManager {
namespace Internal {

ROSSettings::ROSSettings()
{
  m_system_distributions.clear();
  Utils::FilePath ros_path = Utils::FilePath::fromString(Constants::ROS_INSTALL_DIRECTORY);
  if (ros_path.exists())
  {
    QDir ros_opt(ros_path.toString());
    ros_opt.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
    for (auto entry : ros_opt.entryList())
    {
      Utils::FilePath path = Utils::FilePath::fromString(QLatin1String(ROSProjectManager::Constants::ROS_INSTALL_DIRECTORY));
      path = path.pathAppended(entry);
      m_system_distributions.append(path);
    }
  }
}

void ROSSettings::toSettings(QSettings *s) const
{
    s->beginGroup(QLatin1String(Constants::ROS_SETTINGS_GROUP_ID));
    s->setValue(DEFAULT_DISTRIBUTION_ID, default_distribution);
    s->setValue(DEFAULT_BUILD_SYSTEM_ID, static_cast<int>(default_build_system));
    s->setValue(DEFAULT_CODE_STYLE_ID, default_code_style);

    if (default_dist_path.isEmpty())
      s->setValue(DEFAULT_DISTRIBUTION_PATH_ID, Constants::ROS_INSTALL_DIRECTORY);
    else
      s->setValue(DEFAULT_DISTRIBUTION_PATH_ID, default_dist_path);

    s->setValue(CUSTOM_DISTRIBUTION_PATH_ID, custom_dist_path);

    s->endGroup();
}

void ROSSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::ROS_SETTINGS_GROUP_ID));

    default_distribution = s->value(DEFAULT_DISTRIBUTION_ID, "").toString();

    if (default_distribution.isEmpty() && !m_system_distributions.empty())
      default_distribution = m_system_distributions.first().toString();

    default_build_system = static_cast<ROSUtils::BuildSystem>(s->value(DEFAULT_BUILD_SYSTEM_ID, static_cast<int>(ROSUtils::BuildSystem::CatkinTools)).toInt());
    default_code_style = s->value(DEFAULT_CODE_STYLE_ID, "ROS").toString();
    default_dist_path = s->value(DEFAULT_DISTRIBUTION_PATH_ID, Constants::ROS_INSTALL_DIRECTORY).toString();

    if (default_dist_path.isEmpty())
      default_dist_path = Constants::ROS_INSTALL_DIRECTORY;

    custom_dist_path = s->value(CUSTOM_DISTRIBUTION_PATH_ID, "").toString();
    s->endGroup();
}

bool ROSSettings::equals(const ROSSettings &rhs) const
{
    return default_distribution == rhs.default_distribution
           && default_build_system == rhs.default_build_system
           && default_code_style == rhs.default_code_style
           && default_dist_path == rhs.default_dist_path
           && custom_dist_path == rhs.custom_dist_path;
}

// ------------------ ROSSettingsWidget

ROSSettingsWidget::ROSSettingsWidget(QWidget *parent) :
    QWidget(parent),
    m_ui(new Ui::ROSSettingsPage),
    m_available_code_style_names(new QStringListModel)
{
    m_ui->setupUi(this);

    // Add available ros distributions
    QStringList installed_distributions;
    for(auto entry : ROSUtils::installedDistributions())
      installed_distributions.append(entry.toString());

    m_ui->distributionComboBox->addItems(installed_distributions);

    if (!installed_distributions.empty())
      m_ui->distributionComboBox->setCurrentIndex(0);

    // See ProjectExplorer::CodeStyleSettingsWidget and ProjectExplorer::EditorConfiguration as an example
    // TODO: Add python support
    TextEditor::CodeStylePool *code_style_pool = TextEditor::TextEditorSettings::codeStylePool(CppTools::Constants::CPP_SETTINGS_ID);

    for (const auto& code_style : code_style_pool->builtInCodeStyles()) {
        QString name = code_style->displayName() + " [built-in]";
        m_available_code_styles[name] = code_style->displayName();
    }

    for (const auto& code_style : code_style_pool->customCodeStyles()) {
        QString name = code_style->displayName();
        if (name != "Global")
            m_available_code_styles[name] = name;
    }

    m_available_code_style_names->setStringList(m_available_code_styles.keys());
    m_ui->codeStyleComboBox->setModel(m_available_code_style_names);
    m_ui->defaultDistributionPathChooser->setPath(Constants::ROS_INSTALL_DIRECTORY);
}

ROSSettingsWidget::~ROSSettingsWidget()
{
    delete m_ui;
}

ROSSettings ROSSettingsWidget::settings() const
{
    ROSSettings rc;
    rc.default_distribution = m_ui->distributionComboBox->currentText();
    rc.default_build_system = static_cast<ROSUtils::BuildSystem>(m_ui->buildSystemComboBox->currentIndex());
    rc.default_code_style = m_available_code_styles[m_ui->codeStyleComboBox->currentText()];
    rc.default_dist_path = m_ui->defaultDistributionPathChooser->path();

    if (rc.default_dist_path.isEmpty())
      rc.default_dist_path = Constants::ROS_INSTALL_DIRECTORY;

    rc.custom_dist_path = m_ui->customDistributionPathChooser->path();
    return rc;
}

void ROSSettingsWidget::setSettings(const ROSSettings &s)
{
    int idx = m_ui->distributionComboBox->findText(s.default_distribution, Qt::MatchExactly);
    m_ui->distributionComboBox->setCurrentIndex(idx);

    m_ui->buildSystemComboBox->setCurrentIndex(static_cast<int>(s.default_build_system));

    for (const auto& key : m_available_code_styles.keys()) {
        if (m_available_code_styles.value(key) == s.default_code_style) {
            idx = m_ui->codeStyleComboBox->findText(key, Qt::MatchExactly);
            m_ui->codeStyleComboBox->setCurrentIndex(idx);
            break;
        }
    }

    if (s.default_dist_path.isEmpty())
      m_ui->defaultDistributionPathChooser->setPath(Constants::ROS_INSTALL_DIRECTORY);
    else
      m_ui->defaultDistributionPathChooser->setPath(s.default_dist_path);

    m_ui->customDistributionPathChooser->setPath(s.custom_dist_path);
}

// --------------- ROSSettingsPage
ROSSettingsPage::ROSSettingsPage(QSharedPointer<ROSSettings> &settings,
                                 QObject *parent) :
    Core::IOptionsPage(parent),
    m_settings(settings)
{
    setId(Constants::ROS_SETTINGS_MAIN_PAGE_ID);
    setDisplayName(QCoreApplication::translate(Constants::ROS_SETTINGS_MAIN_PAGE_NAME_ID, "ROS Main Settings"));
    setCategory(Constants::ROS_SETTINGS_CATEGORY_ID);
    setDisplayCategory(QCoreApplication::translate(Constants::ROS_SETTINGS_CATEGORY_NAME_ID, "ROS"));
    setCategoryIcon(Utils::Icon({{":rosproject/ros_icon.png", Utils::Theme::PanelTextColorDark}}, Utils::Icon::Tint));
}

QWidget *ROSSettingsPage::widget()
{

    if (!m_widget) {
        m_widget = new ROSSettingsWidget;
        m_widget->setSettings(*m_settings);
    }
    return m_widget;
}

void ROSSettingsPage::apply()
{
    if (m_widget) {
        const ROSSettings newSettings = m_widget->settings();
        if (newSettings != *m_settings) {
            *m_settings = newSettings;
            m_settings->toSettings(Core::ICore::settings());
        }
    }
}

void ROSSettingsPage::finish()
{
    delete m_widget;
}

} // namespace Internal
} // namespace ROSProjectManager
