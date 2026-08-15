#pragma once

#include "rocklaunch/core/runtimes/runtime_source.h"

namespace rocklaunch
{

class SystemWineSource final : public IRuntimeSource
{
public:
    std::vector<Runtime> Discover() const override;
};

} // namespace rocklaunch
