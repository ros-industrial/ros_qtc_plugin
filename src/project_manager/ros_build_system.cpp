#include "ros_build_system.h"

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

ROSBuildSystem::ROSBuildSystem(ROSBuildConfiguration *bc)
    : BuildSystem((BuildConfiguration*)bc)
{
    //
}

void ROSBuildSystem::triggerParsing()
{
    guardParsingRun().markAsSuccess();
}

} // namespace Internal
} // namespace ROSProjectManager
