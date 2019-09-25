/**
 * @author Levi Armstrong
 * @date February 7, 2017
 *
 * @copyright Copyright (c) 2017, Southwest Research Institute
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

#include "ros_packagexml_parser.h"
#include <coreplugin/messagemanager.h>
#include <QFile>
#include <QDebug>

namespace ROSProjectManager {
namespace Internal {

bool ROSPackageXmlParser::parsePackageXml(const Utils::FilePath &filepath)
{
    m_packageInfo.path = filepath.parentDir();
    m_packageInfo.filepath = filepath;
    m_packageInfo.buildFile = m_packageInfo.path.pathAppended("CMakeLists.txt");

    QFile pkgFile(filepath.toString());
    if (pkgFile.exists() && pkgFile.open(QFile::ReadOnly)) {
        setDevice(&pkgFile);

        while (!atEnd()) {
            readNext();
            if (name() == "package")
                parse();
            else if (isStartElement())
                parseUnknownElement();
        }

        pkgFile.close();
        return true;
    }

    Core::MessageManager::write(QObject::tr("[ROS Error] Failed to parse file: %1.").arg(m_packageInfo.filepath.toString()));
    return false;
}

bool ROSPackageXmlParser::parsePackageXml(const Utils::FilePath &filepath, ROSUtils::PackageInfo &packageInfo)
{
    bool result = parsePackageXml(filepath);
    packageInfo = m_packageInfo;
    return result;
}

void ROSPackageXmlParser::parse()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == "name")
            parseName();
        else if (name() == "version")
            parseVersion();
        else if (name() == "description")
            parseDescription();
        else if (name() == "maintainer")
            parseMaintainer();
        else if (name() == "license")
            parseLicense();
        else if (name() == "depend")
            parseDepend();
        else if (name() == "build_depend")
            parseBuildDepend();
        else if (name() == "buildtool_depend")
            parseBuildToolDepend();
        else if (name() == "build_export_depend")
            parseBuildExportDepend();
        else if (name() == "exec_depend")
            parseExecDepend();
        else if (name() == "run_depend")
            parseExecDepend();
        else if (name() == "test_depend")
            parseTestDepend();
        else if (name() == "doc_depend")
            parseDocDepend();
        else if (name() == "export")
            parseExport();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void ROSPackageXmlParser::parseName()
{
    m_packageInfo.name = readElementText().trimmed();
}

void ROSPackageXmlParser::parseVersion()
{
    m_packageInfo.version = readElementText().trimmed();
}

void ROSPackageXmlParser::parseDescription()
{
    m_packageInfo.description = readElementText().trimmed();
}

void ROSPackageXmlParser::parseMaintainer()
{
    m_packageInfo.maintainer = readElementText().trimmed();
}

void ROSPackageXmlParser::parseLicense()
{
    m_packageInfo.license = readElementText().trimmed();
}

void ROSPackageXmlParser::parseDepend()
{
    QString value = readElementText().trimmed();
    if (!m_packageInfo.buildDepends.contains(value))
        m_packageInfo.buildDepends.push_back(value);

    if (!m_packageInfo.buildExportDepends.contains(value))
        m_packageInfo.buildExportDepends.push_back(value);

    if (!m_packageInfo.execDepends.contains(value))
        m_packageInfo.execDepends.push_back(value);
}

void ROSPackageXmlParser::parseBuildDepend()
{
    QString value = readElementText().trimmed();
    if (!m_packageInfo.buildDepends.contains(value))
        m_packageInfo.buildDepends.push_back(value);
}

void ROSPackageXmlParser::parseBuildToolDepend()
{
    m_packageInfo.buildToolDepend = readElementText().trimmed();
}

void ROSPackageXmlParser::parseBuildExportDepend()
{
    QString value = readElementText().trimmed();
    if (!m_packageInfo.buildExportDepends.contains(value))
        m_packageInfo.buildExportDepends.push_back(value);
}

void ROSPackageXmlParser::parseExecDepend()
{
    QString value = readElementText().trimmed();
    if (!m_packageInfo.execDepends.contains(value))
        m_packageInfo.execDepends.push_back(value);
}

void ROSPackageXmlParser::parseTestDepend()
{
    QString value = readElementText().trimmed();
    if (!m_packageInfo.testDepends.contains(value))
        m_packageInfo.testDepends.push_back(value);
}

void ROSPackageXmlParser::parseDocDepend()
{
    QString value = readElementText().trimmed();
    if (!m_packageInfo.docDepends.contains(value))
        m_packageInfo.docDepends.push_back(value);
}

void ROSPackageXmlParser::parseExport()
{
    while (!atEnd()) {
        readNext();
        if (isEndElement())
            return;
        else if (name() == "metapackage")
            parseMetapackage();
        else if (isStartElement())
            parseUnknownElement();
    }
}

void ROSPackageXmlParser::parseMetapackage()
{
    m_packageInfo.metapackage = true;
}

void ROSPackageXmlParser::parseUnknownElement()
{
    Q_ASSERT(isStartElement());

    while (!atEnd()) {
        readNext();

        if (isEndElement())
            break;

        if (isStartElement())
            parseUnknownElement();
    }
}

ROSUtils::PackageInfo ROSPackageXmlParser::getInfo() const
{
    return m_packageInfo;
}


} // namespace Internal
} // namespace ROSProjectManager
