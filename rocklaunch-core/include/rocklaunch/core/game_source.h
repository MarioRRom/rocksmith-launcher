#pragma once

#include "rocklaunch/core/game_profile.h"

#include <optional>

namespace rocklaunch
{

// Strategy for locating a game installation, hiding where it came from.
class GameSource
{
public:
    virtual ~GameSource() = default;

    virtual std::optional<fs::path> Locate(const IGameProfile &profile) const = 0;
};

} // namespace rocklaunch
