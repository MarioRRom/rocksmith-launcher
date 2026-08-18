#pragma once

#include "rocklaunch/core/config_store.h"
#include "rocklaunch/core/patches/launch_patch.h"

#include <memory>
#include <string>
#include <vector>

namespace rocklaunch
{

// Owns the known patches and applies them to profiles. Every operation first
// verifies that the patch belongs to the profile's game before touching the
// install or prefix (mirrors RunnerManager's role for runners).
class PatchManager
{
public:
    PatchManager(ConfigStore &configStore, std::vector<std::unique_ptr<ILaunchPatch>> patches);

    std::vector<const ILaunchPatch *> List() const;
    const ILaunchPatch *Find(const std::string &patchId) const;
    // Enables the patch on the profile and persists the toggle. When force is
    // true, re-applies even if already enabled. Returns false (with error
    // filled) when the profile or patch is unknown or incompatible.
    bool Enable(const std::string &profileId, const std::string &patchId,
                std::string &error, bool force = false);
    // Disables the patch on the profile and persists the toggle.
    bool Disable(const std::string &profileId,
                 const std::string &patchId,
                 std::string &error,
                 bool force = false);

    static PatchManager CreateDefault(ConfigStore &configStore);

private:
    ConfigStore &m_configStore;
    std::vector<std::unique_ptr<ILaunchPatch>> m_patches;
};

} // namespace rocklaunch
