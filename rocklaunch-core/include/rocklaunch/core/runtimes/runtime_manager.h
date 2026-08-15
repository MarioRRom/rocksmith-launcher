#pragma once

#include "rocklaunch/core/runtimes/runtime_source.h"

#include <memory>
#include <optional>
#include <vector>

namespace rocklaunch
{

class RuntimeManager
{
public:
    explicit RuntimeManager(std::vector<std::unique_ptr<IRuntimeSource>> sources);

    std::vector<Runtime> List() const;
    std::optional<Runtime> Find(const std::string &runtimeId) const;

    static RuntimeManager CreateDefault();

private:
    std::vector<std::unique_ptr<IRuntimeSource>> m_sources;
};

} // namespace rocklaunch
