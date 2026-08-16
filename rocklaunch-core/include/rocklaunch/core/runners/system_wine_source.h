#pragma once

#include "rocklaunch/core/runners/runner_source.h"

namespace rocklaunch
{

class SystemWineSource final : public IRunnerSource
{
public:
    std::vector<Runner> Discover() const override;
};

} // namespace rocklaunch
