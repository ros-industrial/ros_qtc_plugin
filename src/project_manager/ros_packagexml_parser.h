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
#ifndef ROS_PACKAGEXML_PARSER_H
#define ROS_PACKAGEXML_PARSER_H

#include "ros_utils.h"

#include <QString>
#include <QXmlStreamReader>

namespace ROSProjectManager {
namespace Internal {

class ROSPackageXmlParser : public QXmlStreamReader
{
public:
    ROSPackageXmlParser() {}

    bool parsePackageXml(const Utils::FilePath &filepath);

    bool parsePackageXml(const Utils::FilePath &filepath,
                         ROSUtils::PackageInfo &packageInfo);

    ROSUtils::PackageInfo getInfo() const;

private:
    void parse();
    void parseName();
    void parseVersion();
    void parseDescription();
    void parseMaintainer();
    void parseLicense();

    void parseDepend();
    void parseBuildDepend();
    void parseBuildToolDepend();
    void parseBuildExportDepend();
    void parseExecDepend();
    void parseTestDepend();
    void parseDocDepend();

    void parseExport();
    void parseMetapackage();

    void parseUnknownElement();

    ROSUtils::PackageInfo m_packageInfo;
};
} // namespace Internal
} // namespace ROSProjectManager

#endif // ROS_PACKAGEXML_PARSER_H
