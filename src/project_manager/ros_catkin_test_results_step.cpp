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

#include "ros_catkin_test_results_step.h"
#include "ros_utils.h"
#include "ros_build_configuration.h"

namespace ROSProjectManager {
namespace Internal {

ROSCatkinTestResultsStep::ROSCatkinTestResultsStep(RunStepList *rsl) : ROSGenericRunStep(rsl, Constants::ROS_CATKIN_TEST_RESULTS_ID)
{
  ctor();
}

ROSCatkinTestResultsStep::ROSCatkinTestResultsStep(RunStepList *rsl, Utils::Id id) : ROSGenericRunStep(rsl, id)
{
  ctor();
}

void ROSCatkinTestResultsStep::ctor()
{
  setCommand("catkin_test_results");
}

RunStepConfigWidget *ROSCatkinTestResultsStep::createConfigWidget()
{
  return new ROSGenericRunStepConfigWidget(this, false, true, false);
}

ROSCatkinTestResultsStepFactory::ROSCatkinTestResultsStepFactory() :
    RunStepFactory()
{
  registerStep<ROSCatkinTestResultsStep>(Constants::ROS_CATKIN_TEST_RESULTS_ID);
  setDisplayName("ROS Catkin Test Results Step");
  setFlags(RunStepInfo::Flags::UniqueStep);
  setSupportedProjectType(Constants::ROS_PROJECT_ID);
  setSupportedStepList(Constants::ROS_RUN_STEP_LIST_ID);
}

} // namespace Internal
} // namespace ROSProjectManager
