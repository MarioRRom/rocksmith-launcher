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
    virtual PatchPreset Preset() const = 0;
    virtual bool IsEnabled(const ProfileConfig &profile) const = 0;
    // When force is true, re-applies even if already enabled (useful for
    // regenerating cache states or recovering from a corrupted apply).
    virtual void Apply(const ProfileConfig &profile, bool force = false) const = 0;
    // Reverts the patch changes when reversible.
    virtual void Remove(const ProfileConfig &profile) const = 0;
};

} // namespace rocklaunch
