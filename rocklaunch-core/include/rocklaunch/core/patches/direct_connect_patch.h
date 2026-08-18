#pragma once

#include "rocklaunch/core/patches/launch_patch.h"

namespace rocklaunch
{

// Phase 4 no-cable patch: enables the game's hidden Direct Connect input mode
// by editing an entry inside cache.psarc (see DirectConnectPatch.md). One-time,
// install-level, reversible via backup.
class DirectConnectPatch final : public ILaunchPatch
{
public:
    std::string Id() const override;
    std::string GameId() const override;
    PatchPreset Preset() const override;
    bool IsEnabled(const ProfileConfig &profile) const override;
    void Apply(const ProfileConfig &profile, bool force = false) const override;
    void Remove(const ProfileConfig &profile) const override;
};

} // namespace rocklaunch
