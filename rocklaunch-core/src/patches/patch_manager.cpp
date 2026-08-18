#include "rocklaunch/core/patches/patch_manager.h"

#include "rocklaunch/core/patches/cdlc_patch.h"
#include "rocklaunch/core/patches/direct_connect_patch.h"

#include <optional>
#include <utility>

namespace rocklaunch
{

namespace
{

// Loads the profile, or fills error and returns nullopt when it does not exist.
std::optional<ProfileConfig> LoadProfileFor(const ConfigStore &configStore,
                                            const std::string &profileId,
                                            std::string &error)
{
    if (!configStore.ProfileExists(profileId)) {
        error = "Profile not found: " + profileId;
        return std::nullopt;
    }

    return configStore.LoadProfile(profileId);
}

} // namespace

PatchManager::PatchManager(ConfigStore &configStore,
                           std::vector<std::unique_ptr<ILaunchPatch>> patches)
    : m_configStore(configStore)
    , m_patches(std::move(patches))
{
}

std::vector<const ILaunchPatch *> PatchManager::List() const
{
    std::vector<const ILaunchPatch *> patches;
    patches.reserve(m_patches.size());
    for (const std::unique_ptr<ILaunchPatch> &patch : m_patches) {
        patches.push_back(patch.get());
    }

    return patches;
}

const ILaunchPatch *PatchManager::Find(const std::string &patchId) const
{
    for (const std::unique_ptr<ILaunchPatch> &patch : m_patches) {
        if (patch->Id() == patchId) {
            return patch.get();
        }
    }

    return nullptr;
}

bool PatchManager::Enable(const std::string &profileId,
                          const std::string &patchId,
                          std::string &error,
                          bool force)
{
    const ILaunchPatch *patch = Find(patchId);
    if (patch == nullptr) {
        error = "Unknown patch: " + patchId;
        return false;
    }

    std::optional<ProfileConfig> profile = LoadProfileFor(m_configStore, profileId, error);
    if (!profile.has_value()) {
        return false;
    }

    // A patch never applies to a profile of a different game — the most
    // important check in the system, enforced at the single manager entry point.
    if (patch->GameId() != profile->gameId) {
        error = "Patch " + patchId + " is for game " + patch->GameId() + ", profile "
              + profileId + " is for game " + profile->gameId;
        return false;
    }

    if (profile->installDir.empty()) {
        error = "Profile " + profileId + " has no install path. "
                "Use set-path <profile> <path> before applying patches.";
        return false;
    }

    if (!force && patch->IsEnabled(*profile)) {
        error = "Patch " + patchId + " is already enabled on profile " + profileId;
        return false;
    }

    patch->Apply(*profile, force);
    profile->patches[patchId].enabled = true;
    m_configStore.SaveProfile(*profile);
    return true;
}

bool PatchManager::Disable(const std::string &profileId,
                           const std::string &patchId,
                           std::string &error,
                           bool force)
{
    const ILaunchPatch *patch = Find(patchId);
    if (patch == nullptr) {
        error = "Unknown patch: " + patchId;
        return false;
    }

    std::optional<ProfileConfig> profile = LoadProfileFor(m_configStore, profileId, error);
    if (!profile.has_value()) {
        return false;
    }

    if (patch->GameId() != profile->gameId) {
        error = "Patch " + patchId + " is for game " + patch->GameId() + ", profile "
              + profileId + " is for game " + profile->gameId;
        return false;
    }

    if (!force && !patch->IsEnabled(*profile)) {
        error = "Patch " + patchId + " is not enabled on profile " + profileId;
        return false;
    }

    patch->Remove(*profile);
    profile->patches[patchId].enabled = false;
    m_configStore.SaveProfile(*profile);
    return true;
}

PatchManager PatchManager::CreateDefault(ConfigStore &configStore)
{
    std::vector<std::unique_ptr<ILaunchPatch>> patches;
    patches.emplace_back(std::make_unique<DirectConnectPatch>());
    patches.emplace_back(std::make_unique<CDLCPatch>());
    return PatchManager(configStore, std::move(patches));
}

} // namespace rocklaunch
