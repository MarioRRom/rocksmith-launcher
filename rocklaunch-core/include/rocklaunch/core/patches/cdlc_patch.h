#pragma once

#include "rocklaunch/core/patches/launch_patch.h"

namespace rocklaunch
{

// Phase 5 CDLC patch: lets custom song charts load by deploying the CDLC enabler
// into the game folder (see CustomDLC.md). Install-level and reversible.
class CDLCPatch final : public ILaunchPatch
{
public:
    std::string Id() const override;
    std::string GameId() const override;
    PatchPreset Preset() const override;
    bool IsEnabled(const ProfileConfig &profile) const override;
    void Apply(const ProfileConfig &profile) const override;
    void Remove(const ProfileConfig &profile) const override;
};

} // namespace rocklaunch
