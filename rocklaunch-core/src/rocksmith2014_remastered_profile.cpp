#include "rocklaunch/core/rocksmith2014_remastered_profile.h"

namespace rocklaunch
{

std::string Rocksmith2014RemasteredProfile::Id() const
{
    return "rocksmith2014remastered";
}

// Only the expected files matter; the launcher does not care how the install was obtained.
bool Rocksmith2014RemasteredProfile::ValidateInstall(const fs::path &installDir) const
{
    std::error_code executableError;
    std::error_code dlcError;
    bool executableExists = fs::is_regular_file(Executable(installDir), executableError);
    bool dlcExists = fs::is_directory(installDir / "dlc", dlcError);
    return executableExists && dlcExists;
}

// Runner mechanics (WINEPREFIX vs STEAM_COMPAT_DATA_PATH) are the launch's job;
// Rocksmith itself needs no extra variables.
std::vector<std::string>
Rocksmith2014RemasteredProfile::RequiredEnv(const LaunchContext &context) const
{
    (void)context;
    return {};
}

fs::path Rocksmith2014RemasteredProfile::Executable(const fs::path &installDir) const
{
    return installDir / "Rocksmith2014.exe";
}

} // namespace rocklaunch
