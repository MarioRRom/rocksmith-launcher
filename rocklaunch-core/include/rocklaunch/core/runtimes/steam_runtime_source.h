#pragma once

#include "rocklaunch/core/runtimes/runtime_source.h"

namespace rocklaunch
{

class SteamRuntimeSource final : public IRuntimeSource
{
public:
    explicit SteamRuntimeSource(std::vector<fs::path> steamRoots = DefaultRoots());

    std::vector<Runtime> Discover() const override;

    static std::vector<fs::path> DefaultRoots();

private:
    std::vector<fs::path> m_steamRoots;
};

} // namespace rocklaunch
