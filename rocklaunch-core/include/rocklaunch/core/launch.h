#pragma once

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/game_profile.h"
#include "rocklaunch/core/runtimes/runtime.h"

#include <string>
#include <vector>

namespace rocklaunch
{

// Command line and environment ready to run the game.
struct LaunchCommand
{
    std::vector<std::string> command;
    std::vector<std::string> environment;
};

// Creates the prefix directory when it does not exist yet and applies the global
// prefix settings (dsound=alsa). Returns warnings for settings that could not be applied.
std::vector<std::string> EnsurePrefix(const fs::path &prefixDir, const Runtime &runtime);

// Builds the command line and environment to run profile's game with the given runtime.
LaunchCommand BuildLaunchCommand(const ProfileConfig &profile,
                                 const Runtime &runtime,
                                 const IGameProfile &game);

// Runs the command, replacing the current process. Returns false when the process
// could not be started; on success the current process no longer exists.
bool ExecLaunchCommand(const LaunchCommand &command);

} // namespace rocklaunch
