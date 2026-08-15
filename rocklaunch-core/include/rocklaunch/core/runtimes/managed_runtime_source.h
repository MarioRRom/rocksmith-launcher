#pragma once

#include "rocklaunch/core/runtimes/runtime_source.h"

namespace rocklaunch
{

class ManagedRuntimeSource final : public IRuntimeSource
{
public:
    explicit ManagedRuntimeSource(fs::path runtimeDir = DefaultRuntimeDir());

    std::vector<Runtime> Discover() const override;

    static fs::path DefaultRuntimeDir();

private:
    fs::path m_runtimeDir;
};

} // namespace rocklaunch
