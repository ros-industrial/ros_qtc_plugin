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

#include "ros_rostest_step.h"
#include "ros_utils.h"
#include "ros_build_configuration.h"

namespace ROSProjectManager {
namespace Internal {

ROSTestStep::ROSTestStep(RunStepList *rsl) : ROSGenericRunStep(rsl, Constants::ROS_TEST_ID)
{
  ctor();
}

ROSTestStep::ROSTestStep(RunStepList *rsl, Utils::Id id) : ROSGenericRunStep(rsl, id)
{
  ctor();
}

void ROSTestStep::ctor()
{
  setCommand("rostest");
}

RunStepConfigWidget *ROSTestStep::createConfigWidget()
{
  return new ROSGenericRunStepConfigWidget(this, true, true, false);
}

QMap<QString, QString> ROSTestStep::getAvailableTargets()
{
  ROSBuildConfiguration *bc = qobject_cast<ROSBuildConfiguration *>(target()->activeBuildConfiguration());
  return ROSUtils::getROSPackageExecutables(getPackage(), bc->environment().toStringList());
}

ROSTestStepFactory::ROSTestStepFactory() :
    RunStepFactory()
{
  registerStep<ROSTestStep>(Constants::ROS_TEST_ID);
  setDisplayName("ROS Test Step");
  setSupportedProjectType(Constants::ROS_PROJECT_ID);
  setSupportedStepList(Constants::ROS_RUN_STEP_LIST_ID);
}

} // namespace Internal
} // namespace ROSProjectManager
