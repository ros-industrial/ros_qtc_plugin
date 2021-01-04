#include "ros_build_system.h"

#include "ros_build_configuration.h"

using namespace ProjectExplorer;

namespace ROSProjectManager {
namespace Internal {

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

ROSBuildSystem::ROSBuildSystem(ROSProject *p)
    : BuildSystem(p)
{
    //
}

bool ROSBuildSystem::validateParsingContext(const ParsingContext &ctx)
{
    return ctx.project && qobject_cast<ROSBuildConfiguration *>(ctx.buildConfiguration);
}

void ROSBuildSystem::parseProject(ParsingContext &&ctx)
{
    ctx.guard.markAsSuccess();
}

} // namespace Internal
} // namespace ROSProjectManager
