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
#ifndef ROS_ROSLAUNCH_STEP_H
#define ROS_ROSLAUNCH_STEP_H

#include "ros_generic_run_step.h"

namespace ROSProjectManager {
namespace Internal {

namespace Ui { class ROSGenericStep; }

class ROSLaunchStep : public ROSGenericRunStep
{
  Q_OBJECT
  friend class ROSLaunchStepFactory;

public:
  ROSLaunchStep(RunStepList *rsl);

protected:
  ROSLaunchStep(RunStepList *rsl, Utils::Id id);

  void ctor();

  RunStepConfigWidget *createConfigWidget() override;

  QMap<QString, QString> getAvailableTargets() override;

};

class ROSLaunchStepFactory : public RunStepFactory
{
public:
    ROSLaunchStepFactory();
};

} // Internal
} // ROSProjectManager
#endif // ROS_ROSLAUNCH_STEP_H
