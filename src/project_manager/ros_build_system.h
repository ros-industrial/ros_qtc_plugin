#pragma once

#include <projectexplorer/buildsystem.h>

namespace ROSProjectManager {
namespace Internal {

class ROSBuildConfiguration;

// --------------------------------------------------------------------
// ROSBuildSystem:
// --------------------------------------------------------------------

class ROSBuildSystem : public ProjectExplorer::BuildSystem
{
    Q_OBJECT

public:
    explicit ROSBuildSystem(ROSBuildConfiguration *bc);

    void triggerParsing() final;
};

} // namespace Internal
} // namespace ROSProjectManager
