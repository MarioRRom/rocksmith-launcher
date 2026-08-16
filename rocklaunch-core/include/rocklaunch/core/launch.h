#pragma once

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/game_profile.h"
#include "rocklaunch/core/runners/runner.h"

#include <string>
#include <vector>

namespace rocklaunch
{

// Command line and environment ready to run the game.
struct LaunchCommand
{
    std::vector<std::string> command;
    std::vector<std::string> environment;
    // Working directory for the launched process; the game's install directory.
    fs::path workingDirectory;
};

// Creates the prefix directory when it does not exist yet and applies the global
// prefix settings (Audio=alsa). Returns warnings for settings that could not be applied.
std::vector<std::string> EnsurePrefix(const fs::path &prefixDir, const Runner &runner);

// Builds the command line and environment to run profile's game with the given runner.
LaunchCommand BuildLaunchCommand(const ProfileConfig &profile,
                                 const Runner &runner,
                                 const IGameProfile &game);

// Runs the command, replacing the current process. Returns false when the process
// could not be started; on success the current process no longer exists.
bool ExecLaunchCommand(const LaunchCommand &command);

} // namespace rocklaunch
