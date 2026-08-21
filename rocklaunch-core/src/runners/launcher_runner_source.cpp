#include "rocklaunch/core/runners/launcher_runner_source.h"

#include "rocklaunch/core/config_store.h"

#include <utility>

namespace rocklaunch
{

LauncherRunnerSource::LauncherRunnerSource(fs::path runnerDir)
    : m_runnerDir(std::move(runnerDir))
{
}

std::vector<Runner> LauncherRunnerSource::Discover() const
{
    std::vector<Runner> runners;
    std::error_code error;
    if (!fs::is_directory(m_runnerDir, error)) {
        return runners;
    }

    for (const fs::directory_entry &entry : fs::directory_iterator(m_runnerDir)) {
        fs::path proton = entry.path() / "proton";
        if (entry.is_directory() && fs::is_regular_file(proton, error)) {
            std::string name = entry.path().filename().string();
            runners.push_back({ name, name,
                                 RunnerType::Proton, "launcher", entry.path(), proton });
        }
    }

    return runners;
}

fs::path LauncherRunnerSource::DefaultRunnerDir()
{
    return ConfigStore::DefaultDataDir() / "runners";
}

} // namespace rocklaunch