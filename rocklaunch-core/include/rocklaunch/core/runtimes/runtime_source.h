#pragma once

#include "rocklaunch/core/runtimes/runtime.h"

#include <vector>

namespace rocklaunch
{

class IRuntimeSource
{
public:
    virtual ~IRuntimeSource() = default;

    virtual std::vector<Runtime> Discover() const = 0;
};

} // namespace rocklaunch
