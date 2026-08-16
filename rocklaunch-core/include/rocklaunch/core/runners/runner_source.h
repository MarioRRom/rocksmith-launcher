#pragma once

#include "rocklaunch/core/runners/runner.h"

#include <vector>

namespace rocklaunch
{

class IRunnerSource
{
public:
    virtual ~IRunnerSource() = default;

    virtual std::vector<Runner> Discover() const = 0;
};

} // namespace rocklaunch
