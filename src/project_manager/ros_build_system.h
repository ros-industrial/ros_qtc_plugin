#pragma once

#include <projectexplorer/buildsystem.h>

namespace ROSProjectManager {
namespace Internal {

class ROSProject;

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

class ROSBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit ROSBuildSystem(ROSProject *project);

protected:
    bool validateParsingContext(const ParsingContext &ctx) final;
    void parseProject(ParsingContext &&ctx) final;
};

} // namespace Internal
} // namespace ROSProjectManager
