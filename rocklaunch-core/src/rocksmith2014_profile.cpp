#include "rocklaunch/core/rocksmith2014_profile.h"

namespace rocklaunch
{

std::string Rocksmith2014Profile::Id() const
{
    return "rocksmith2014";
}

// Only the expected files matter; the launcher does not care how the install was obtained.
bool Rocksmith2014Profile::ValidateInstall(const fs::path &installDir) const
{
    std::error_code executableError;
    std::error_code dlcError;
    bool executableExists = fs::is_regular_file(Executable(installDir), executableError);
    bool dlcExists = fs::is_directory(installDir / "dlc", dlcError);
    return executableExists && dlcExists;
}

// Point the Steam compatibility layer at the profile's prefix.
std::vector<std::string> Rocksmith2014Profile::RequiredEnv(const LaunchContext &context) const
{
    std::vector<std::string> environment;

    if (!context.prefixDir.empty()) {
        environment.emplace_back("STEAM_COMPAT_DATA_PATH=" + context.prefixDir.string());
    }

    return environment;
}

fs::path Rocksmith2014Profile::Executable(const fs::path &installDir) const
{
    return installDir / "Rocksmith2014.exe";
}

} // namespace rocklaunch
