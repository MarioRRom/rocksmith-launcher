#include "rocklaunch/core/patches/cdlc_patch.h"

#include <stdexcept>

namespace rocklaunch
{

std::string CDLCPatch::Id() const
{
    return "cdlc";
}

std::string CDLCPatch::GameId() const
{
    return "rocksmith2014remastered";
}

PatchPreset CDLCPatch::Preset() const
{
    return {
        Id(),
        GameId(),
        "Custom Songs (CDLC)",
        "Lets custom song charts (CDLC) load by deploying the CDLC enabler DLL "
        "into the game folder, removing it when disabled.",
        true,
        true,
        {
            { PatchOperationType::CopyFile, fs::path("d3dx9_42.dll"),
              "deploy the CDLC enabler DLL next to the game executable" },
            { PatchOperationType::RemoveFile, fs::path("d3dx9_42.dll"),
              "remove the enabler DLL when disabled" },
        },
    };
}

bool CDLCPatch::IsEnabled(const ProfileConfig &profile) const
{
    const auto it = profile.patches.find(Id());
    return it != profile.patches.end() && it->second.enabled;
}

void CDLCPatch::Apply(const ProfileConfig &profile, bool force) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    // TODO(phase 5): deploy the CDLC enabler (see CustomDLCPatch.md). The source of
    // the DLL (download-on-demand vs user-provided) is not decided yet.
}

void CDLCPatch::Remove(const ProfileConfig &profile) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    // TODO(phase 5): remove the deployed enabler DLL from the game folder.
}

} // namespace rocklaunch
