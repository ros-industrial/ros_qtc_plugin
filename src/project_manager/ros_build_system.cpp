#include "ros_build_system.h"
#include "ros_build_configuration.h"

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

ROSBuildSystem::ROSBuildSystem(ROSBuildConfiguration *bc)
    : BuildSystem((BuildConfiguration*)bc)
{
    connect(((BuildConfiguration*)bc)->project(), &Project::activeTargetChanged, this, [this]() { triggerParsing(); });
}

void ROSBuildSystem::triggerParsing()
{
    guardParsingRun().markAsSuccess();
}

} // namespace Internal
} // namespace ROSProjectManager
