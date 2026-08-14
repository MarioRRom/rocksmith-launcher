#pragma once

#include "rocklaunch/core/game_source.h"

namespace rocklaunch
{

// A game location supplied directly by the user, typically a non-Steam installation.
class ManualSource final : public GameSource
{
public:
    explicit ManualSource(fs::path installDir);

    std::optional<fs::path> Locate(const IGameProfile &profile) const override;

private:
    fs::path m_installDir;
};

} // namespace rocklaunch
