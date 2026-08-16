#pragma once

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/patches/patch.h"

#include <string>

namespace rocklaunch
{

// A patch modifies a game installation so the game gains a feature (no-cable,
// CDLC). Each patch targets exactly one game and refuses profiles of any other
// game before touching the install or prefix. PatchManager owns the instances;
// a patch carries no per-profile state.
class ILaunchPatch
{
public:
    virtual ~ILaunchPatch() = default;

    // Unique patch id, e.g. "direct-connect".
    virtual std::string Id() const = 0;
    // Game the patch was designed for; profiles with a different game_id are rejected.
    virtual std::string GameId() const = 0;
    // Description of what the patch does and what it touches.
    virtual PatchPreset Preset() const = 0;
    // Whether the profile currently has this patch enabled.
    virtual bool IsEnabled(const ProfileConfig &profile) const = 0;
    // Enables the patch for the profile. Each patch re-verifies profile.gameId
    // before doing anything; throws std::runtime_error on mismatch or failure.
    virtual void Apply(const ProfileConfig &profile) const = 0;
    // Disables the patch for the profile, reverting its changes when reversible.
    virtual void Remove(const ProfileConfig &profile) const = 0;
};

} // namespace rocklaunch
