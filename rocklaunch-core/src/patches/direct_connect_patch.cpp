#include "rocklaunch/core/patches/direct_connect_patch.h"

#include <stdexcept>

namespace rocklaunch
{

std::string DirectConnectPatch::Id() const
{
    return "direct-connect";
}

std::string DirectConnectPatch::GameId() const
{
    return "rocksmith2014remastered";
}

PatchPreset DirectConnectPatch::Preset() const
{
    return {
        Id(),
        GameId(),
        "Direct Connect Mode",
        "Enables the game's hidden Input > Direct Connect mode so any audio "
        "device can be used as the guitar input. Edits cache.psarc once and "
        "backs the original up to cache.bak.",
        true,
        true,
        {
            { PatchOperationType::EditFile, fs::path("cache.psarc"),
              "toggle the Direct Connect flag inside cache7.7z manifests" },
            { PatchOperationType::RestoreFile, fs::path("cache.psarc"),
              "restore the backed-up original when disabled" },
        },
    };
}

bool DirectConnectPatch::IsEnabled(const ProfileConfig &profile) const
{
    const auto it = profile.patches.find(Id());
    return it != profile.patches.end() && it->second.enabled;
}

void DirectConnectPatch::Apply(const ProfileConfig &profile) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    // TODO(phase 4): back up cache.psarc and apply the cache7.7z manifest edit
    // (see NoCablePatch.md). Reimplement the change against the user's archive.
}

void DirectConnectPatch::Remove(const ProfileConfig &profile) const
{
    if (profile.gameId != GameId()) {
        throw std::runtime_error("Patch " + Id() + " is for game " + GameId()
                                 + ", profile is for game " + profile.gameId);
    }

    // TODO(phase 4): restore the backed-up original cache.psarc.
}

} // namespace rocklaunch
