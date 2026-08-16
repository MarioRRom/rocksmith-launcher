#include "rocklaunch/core/runners/steam_runner_source.h"

#include <cstdlib>
#include <set>

namespace rocklaunch
{

namespace
{

void AppendRunnerDirectories(const fs::path &directory, std::vector<Runner> &runners)
{
    std::error_code error;
    if (!fs::is_directory(directory, error)) {
        return;
    }

    for (const fs::directory_entry &entry : fs::directory_iterator(directory)) {
        fs::path proton = entry.path() / "proton";
        if (entry.is_directory() && fs::is_regular_file(proton, error)) {
            std::string name = entry.path().filename().string();
            runners.push_back({ "steam-proton-" + SanitizeRunnerId(name), name,
                                 RunnerType::Proton, "steam", entry.path(), proton });
        }
    }
}

} // namespace

SteamRunnerSource::SteamRunnerSource(std::vector<fs::path> steamRoots)
    : m_steamRoots(std::move(steamRoots))
{
}

std::vector<Runner> SteamRunnerSource::Discover() const
{
    std::vector<Runner> runners;
    for (const fs::path &steamRoot : m_steamRoots) {
        AppendRunnerDirectories(steamRoot / "compatibilitytools.d", runners);
    }
    return runners;
}

std::vector<fs::path> SteamRunnerSource::DefaultRoots()
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
