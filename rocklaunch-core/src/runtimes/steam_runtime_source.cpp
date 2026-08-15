#include "rocklaunch/core/runtimes/steam_runtime_source.h"

#include <cstdlib>
#include <set>

namespace rocklaunch
{

namespace
{

void AppendRuntimeDirectories(const fs::path &directory, std::vector<Runtime> &runtimes)
{
    std::error_code error;
    if (!fs::is_directory(directory, error)) {
        return;
    }

    for (const fs::directory_entry &entry : fs::directory_iterator(directory)) {
        fs::path proton = entry.path() / "proton";
        if (entry.is_directory() && fs::is_regular_file(proton, error)) {
            std::string name = entry.path().filename().string();
            runtimes.push_back({ "steam-proton-" + SanitizeRuntimeId(name), name,
                                 RuntimeType::Proton, "steam", entry.path(), proton });
        }
    }
}

} // namespace

SteamRuntimeSource::SteamRuntimeSource(std::vector<fs::path> steamRoots)
    : m_steamRoots(std::move(steamRoots))
{
}

std::vector<Runtime> SteamRuntimeSource::Discover() const
{
    std::vector<Runtime> runtimes;
    for (const fs::path &steamRoot : m_steamRoots) {
        AppendRuntimeDirectories(steamRoot / "compatibilitytools.d", runtimes);
    }
    return runtimes;
}

std::vector<fs::path> SteamRuntimeSource::DefaultRoots()
{
    std::vector<fs::path> roots;
    const char *home = std::getenv("HOME");
    if (home != nullptr) {
        roots.emplace_back(fs::path(home) / ".steam" / "steam");
        roots.emplace_back(fs::path(home) / ".local" / "share" / "Steam");
    }
    return roots;
}

} // namespace rocklaunch
